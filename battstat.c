#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <strings.h>

#define BATDIR "/sys/class/power_supply/BAT0/"

FILE *status_f, *capacity_f, *power_now_f, *energy_now_f;
char status, oldstatus = 'U'; // U = Unknown (hasn't been polled yet),
                              // C = Charging
                              // D = Discharging
uint64_t energy, power;
int capacity;
uint64_t hour, min, sec;
uint64_t oldhour = 99, oldmin = 59, oldsec = 59; // set these to values that
                                                 // won't trigger the low
                                                 // batttery notifications.
char status_line[256]; // line to actually output.

void close_if_open(FILE *f) {
    if(f) {
        fclose(f);
    }
}

void close_files(void) {
    close_if_open(status_f);
    close_if_open(capacity_f);
    close_if_open(power_now_f);
    close_if_open(energy_now_f);
}

int send_notification(char *urgency, char *summary, char *body) {
    int status;
    char *argv[6];
    argv[0] = "/usr/bin/notify-send";
    argv[1] = "-u";
    argv[2] = urgency;
    argv[3] = summary;
    argv[4] = body;
    argv[5] = NULL;
    switch(fork()) {
    case -1:
        return -1;
    case 0:
        return execv("/usr/bin/notify-send", argv);
    default:
        wait(&status);
        return status;
    }
}

int main(int argc, char **argv) {
    for(;;) {

        if(oldstatus != 'U') {
            // wait; we don't want to poll too often..
            // We skip this on the first iteration, as it gives us faster
            // startup. We don't want to just put it at the end of the loop,
            // since then in the event of an error we would start burning up
            // resources (the sleep would be skipped by the contiue
            // statements.)
            sleep(1);
        }

        // clear output from last time;
        bzero(status_line, sizeof status_line);

        // open all the requsite files
        status_f     = fopen(BATDIR"status", "r");
        capacity_f   = fopen(BATDIR"capacity", "r");
        power_now_f  = fopen(BATDIR"power_now", "r");
        energy_now_f = fopen(BATDIR"energy_now", "r");

        // if there are any failures, clean up and try again.
        if(!status_f || !capacity_f || !power_now_f || !energy_now_f) {
            close_files();
            continue;
        }

        // read in the necessary data. again, on failure we clean up and try
        // again.
        if(!fscanf(status_f,     "%c",  &status) ||
           !fscanf(capacity_f,   "%d",  &capacity) ||
           !fscanf(power_now_f,  "%ld", &power) ||
           !fscanf(energy_now_f, "%ld", &energy)) {
            close_files();
            continue;
        }

        // This can happen for brief periods when we've just plugged in,
        // unplugged the charger. We should just give it a bit more time.
        if(power == 0) {
            close_files();
            continue;
        }

        // work out how much time we have left.
        hour = energy/power;
        energy %= power;
        min = (60*energy)/power;
        energy = (60*energy)%power;
        sec = (60*energy)/power;

        // build the status line.
        snprintf(status_line, sizeof status_line,
            (status == 'C')? "%s (%d%%)" : "%s (%d%%), %lu:%02lu:%02lu",
            (status == 'C')? "Charging" : "Discharging",
            capacity, hour, min, sec);

        // clean up after ourselves.
        close_files();

        // output the status.
        puts(status_line);
        fflush(stdout);

        // check for low battery states, and notify if need be.
        // We don't want to report low battery if we're on AC.
        if(status != 'C' && hour == 0) {
            // We don't want to give the same notification repeatedly, so
            // check that last time through we didn't have a low battery.
            if(min < 5 && oldmin >= 5) {
                send_notification("critical", "Very Low Battery", status_line);
            } else if(min < 20 && oldmin >= 20) {
                send_notification("normal", "Low Battery", status_line);
            }
        }
        oldhour = hour;
        oldmin = min;
        oldsec = sec;

        // If the ac adapter has been plugged in or unplugged, notify us.
        if(oldstatus != 'U' && oldstatus != status) {
            if(status == 'C') {
                send_notification("normal", "AC adapter plugged in", status_line);
            } else if(status == 'D') {
                send_notification("normal", "AC adapter unplugged", status_line);
            }
        }
        oldstatus = status;
    }
}

/* vim:set ts=4 sw=4 tw=78 et: */
