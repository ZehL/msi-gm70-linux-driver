#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <dirent.h>
#include <math.h>

#define SCALE_MULTIPLIER 0.6f  

int find_msi_node(void) {
    struct dirent *entry;
    DIR *dp = opendir("/dev/input");
    if (!dp) return -1;

    char path[256];
    char name[256];

    while ((entry = readdir(dp))) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);
            int fd = open(path, O_RDONLY);
            if (fd >= 0) {
                ioctl(fd, EVIOCGNAME(sizeof(name)), name);
                if (strstr(name, "MSI GM70 Gaming Mouse") != NULL) {
                    struct input_absinfo abs;
                    /* Interface 02 handles relative deltas but lacks ABS endpoints */
                    if (ioctl(fd, EVIOCGABS(ABS_X), &abs) < 0) {
                        closedir(dp);
                        return fd; 
                    }
                }
                close(fd);
            }
        }
    }
    closedir(dp);
    return -1;
}

int main(void) {
    int raw_fd = find_msi_node();
    if (raw_fd < 0) {
        fprintf(stderr, "Error: MSI GM70 wireless endpoint not found.\n");
        return 1;
    }

    if (ioctl(raw_fd, EVIOCGRAB, 1) < 0) {
        fprintf(stderr, "Error: Failed to grab device interface.\n");
        close(raw_fd);
        return 1;
    }

    int uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (uinput_fd < 0) {
        fprintf(stderr, "Error: Cannot open /dev/uinput (requires root).\n");
        ioctl(raw_fd, EVIOCGRAB, 0);
        close(raw_fd);
        return 1;
    }

    ioctl(uinput_fd, UI_SET_EVBIT, EV_REL);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_X);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_Y);
    ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_MIDDLE);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_SIDE);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_EXTRA);

    struct uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "MSI-GM70-Wireless-Fix");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x0d22;
    uidev.id.product = 0x0d70;
    uidev.id.version = 1;

    if (write(uinput_fd, &uidev, sizeof(uidev)) < 0) {
        perror("Error writing uinput device structural layouts");
        goto cleanup;
    }
    
    if (ioctl(uinput_fd, UI_DEV_CREATE) < 0) {
        perror("Error creating uinput device endpoint loop");
        goto cleanup;
    }

    struct input_event ev;
    struct input_event out_ev;
    
    int accum_x = 0;
    int accum_y = 0;
    float remainder_x = 0.0f;
    float remainder_y = 0.0f;

    while (read(raw_fd, &ev, sizeof(struct input_event)) > 0) {
        if (ev.type == EV_REL) {
            if (ev.code == REL_X) {
                accum_x += ev.value;
            } else if (ev.code == REL_Y) {
                accum_y += ev.value;
            }
        } 
        else if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
            float target_x = ((float)accum_x * SCALE_MULTIPLIER) + remainder_x;
            float target_y = ((float)accum_y * SCALE_MULTIPLIER) + remainder_y;

            int dispatch_x = (int)truncf(target_x);
            int dispatch_y = (int)truncf(target_y);

            remainder_x = target_x - (float)dispatch_x;
            remainder_y = target_y - (float)dispatch_y;

            if (dispatch_x != 0) {
                memset(&out_ev, 0, sizeof(struct input_event));
                out_ev.type = EV_REL;
                out_ev.code = REL_X;
                out_ev.value = dispatch_x;
                if (write(uinput_fd, &out_ev, sizeof(struct input_event)) < 0) break;
            }
            if (dispatch_y != 0) {
                memset(&out_ev, 0, sizeof(struct input_event));
                out_ev.type = EV_REL;
                out_ev.code = REL_Y;
                out_ev.value = dispatch_y;
                if (write(uinput_fd, &out_ev, sizeof(struct input_event)) < 0) break;
            }
            
            if (dispatch_x != 0 || dispatch_y != 0) {
                memset(&out_ev, 0, sizeof(struct input_event));
                out_ev.type = EV_SYN;
                out_ev.code = SYN_REPORT;
                if (write(uinput_fd, &out_ev, sizeof(struct input_event)) < 0) break;
            }

            accum_x = 0;
            accum_y = 0;
        }
    }

cleanup:
    ioctl(uinput_fd, UI_DEV_DESTROY);
    close(uinput_fd);
    ioctl(raw_fd, EVIOCGRAB, 0);
    close(raw_fd);
    return 0;
}
