/**
 ** clock.c
 **
 **/
#include <gpiod.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>

#include "framebuffer.h"
#include "glyphs.h"

#define IN  0

#define LOW  0
#define HIGH 1

const char      gpio_set[2][4] = { {23, 24, 0, 0}, {18, 27, 22, 23} };

static const unsigned char bitmask[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };

unsigned char   flags = 0x00;

const unsigned short fg1[] = { Red, Green, Blue, Black };
const unsigned short fg2[] = { Pink, DarkGreen, Navy, Black };
const unsigned short al[] = { Yellow, Red, Red, Black };
enum { cs_red, cs_grn, cs_blu, cs_blk };

static char    *getIP()
{
    struct ifaddrs *ifaddr,
                   *ifa;
    int             family,
                    s;
    static char     host[NI_MAXHOST] = "999.999.999.999";

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return host;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host,
                        NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if ((strcmp(ifa->ifa_name, "wlan0") == 0)
            && (ifa->ifa_addr->sa_family == AF_INET))
        {
            if (s != 0)
            {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                return host;
            }
            break;
        }
    }

    freeifaddrs(ifaddr);
    return host;
}

int main(int argc, char *argv[])
{
    struct gpiod_chip *chip;
    struct gpiod_line *gpio_pin[4];

    time_t          now,
                    then = 0;
    struct tm      *now_tm;
    char            now_str[6] = "99:99";
    char            date_str[15] = "";
    char            ap_str[3] = "99";
    const char      suffix[4][3] = { "th", "st", "nd", "rd" };;
    int             cc = 0;
    char            color_scheme = cs_red;
    char           *ip = NULL;
    bool            refresh = false;
    char            gpio = 0;

    enum { th, st, nd, rd };
    char            si;

    enum { flip_flop, hrs24_12, btn_a_latch, alarm, btn_b_latch,
        btn_ab_latch
    };
    ip = getIP();

    // Set up framebuffer
    if (cc = fb_open())
        return (cc);
    force_180 = true;

    for (int i = 1; i < argc; i++)
    {
        if (!strcmp("--gpio=2", argv[i]))
            gpio = 2;
        if (!strcmp("--gpio=4", argv[i]))
            gpio = 4;
    }

    if (gpio)
    {
        int cc;
        chip = gpiod_chip_open_by_name("gpiochip0");
        if (chip == NULL)
        {
            cc = errno;
            printf("gpiod_chip_open_by_name() returned %d\n", cc);
            exit(cc);
        }
        // assumes gpio is 2 or 4 when set!
        for (int i = 0, j = (gpio / 4); i < gpio && gpio_set[j][i]; i++)
        {
            // Enable GPIO pins
            gpio_pin[i] = gpiod_chip_get_line(chip, gpio_set[j][i]);
            if (gpio_pin[i] == NULL)
            {
                cc = errno;
                printf("gpiod_chip_get_line(%d) returned %d\n", gpio_set[j][i], cc);
                exit(cc);
            }
            // Set GPIO directions
            cc = gpiod_line_request_input(gpio_pin[i], argv[0]);
            if (cc)
            {
                printf("gpiod_line_request_input(%d) returned %d\n", gpio_set[j][i], cc);
                exit(cc);
            }
        }
    }

    fb_clear();
    draw_fstr(0, 230, ip, White, 1);
    while (1)
    {
        time(&now);

        if (now > then || refresh)
        {
            if (!refresh)
            {
                flags ^= bitmask[flip_flop];    // next flip-flop
                refresh = (flags & bitmask[flip_flop]);
            }
            if (refresh)
            {                   // because flip-flop set, or forced
                refresh = false;
                // fb_clear();
                // draw_fstr( 0, 230, ip, White, 1);
                box(0, 0, 239, 88, Black);
                now_tm = localtime(&now);
                strftime(date_str, 14, "%a. %b. %d", now_tm);
                draw_fstr(0, 72, date_str, fg1[color_scheme], 2);
                si = (now_tm->tm_mday == 11
                      || (now_tm->tm_mday % 10 >
                          3)) ? 0 : now_tm->tm_mday % 10;
                draw_fstr(215, 72, (char *) suffix[si], fg1[color_scheme],
                          1);
                if (flags & bitmask[hrs24_12])
                {               // 24/12 is 24
                    strftime(now_str, 5, "%H:%M", now_tm);      // 24
                    draw_glyph(213, 25, &glyph_24, fg1[color_scheme], 2);
                }
                else
                {
                    strftime(now_str, 6, "%I:%M", now_tm);      // 12
                    if (now_str[0] == '0')
                        now_str[0] = ' ';
                    strftime(ap_str, 3, "%p", now_tm);
                    if (ap_str[0] == 'A')
                        draw_glyph(213, 25, &glyph_AM, fg1[color_scheme],
                                   2);
                    else
                        draw_glyph(213, 25, &glyph_PM, fg1[color_scheme],
                                   2);
                }
                draw_gstr(0, 5, now_str, fg1[color_scheme], 2);
            }
            else
            {
                draw_glyph(96, 5, &glyph_colon, fg2[color_scheme], 2);
            }
            then = now;
        }

        if (gpio == 2)
        {
            if (LOW == gpiod_line_get_value(gpio_pin[0])   // button A pressed
                &&                                         // AND
                LOW == gpiod_line_get_value(gpio_pin[1]))  // button B pressed
            {
#if 0
                color_scheme++;
                color_scheme %= 4;
#endif
                draw_fstr(0, 102, "Menu", fg1[color_scheme], 2);
            }
            else 
            if (LOW == gpiod_line_get_value(gpio_pin[0])   // button A pressed
                ||                                         // OR
                LOW == gpiod_line_get_value(gpio_pin[1]))  // button B pressed
            {

                if (flags & bitmask[btn_ab_latch])
                {               // button A&B latched?
                    flags ^= bitmask[btn_ab_latch];     // unlatch button
                    // A&B
                }

                if (gpiod_line_get_value(gpio_pin[0]))     // button A pressed
                {
                    if (!(flags & bitmask[btn_a_latch]))
                    {           // button A not latched?
                        box(213, 25, 213 + (glyph_24.bitswide * 2),
                            25 + (glyph_24.bitshigh * 2), Black);
                        flags ^= bitmask[hrs24_12];     // toggle 12/24
                        flags |= bitmask[btn_a_latch];  // latch button A

                        // force refresh
                        refresh = true;
                    }
                }
                else if (flags & bitmask[btn_a_latch])
                {               // button A latched?
                    flags ^= bitmask[btn_a_latch];      // unlatch button
                    // A
                }

                if (gpiod_line_get_value(gpio_pin[1]))     // button B pressed
                {               // button B pressed
                    if (!(flags & bitmask[btn_b_latch]))
                    {           // button B not latched
                        if (flags & bitmask[alarm])
                        {       // ALARM on? (turning off)
                            draw_glyph(219, 5, &glyph_bell, Black, 2);
                        }
                        flags ^= bitmask[alarm];        // toggle ALARM
                        // on/off
                        flags |= bitmask[btn_b_latch];  // latch button B
                    }
                }
                else if (flags & bitmask[btn_b_latch])
                {               // button B latched?
                    flags ^= bitmask[btn_b_latch];      // unlatch button
                    // B
                }

                if (flags & bitmask[alarm])
                {               // ALARM on?
                    draw_glyph(219, 5, &glyph_bell, al[color_scheme], 2);
                }
            }
            else
            {                   // neither pressed
                if (flags & bitmask[btn_a_latch])
                {               // button A latched?
                    flags ^= bitmask[btn_a_latch];      // unlatch button
                    // A
                }
                if (flags & bitmask[btn_b_latch])
                {               // button B latched?
                    flags ^= bitmask[btn_b_latch];      // unlatch button
                    // B
                }
                if (flags & bitmask[btn_ab_latch])
                {               // button A&B latched?
                    flags ^= bitmask[btn_ab_latch];     // unlatch button
                    // A&B
                }
            }
            if (flags & bitmask[alarm])
            {                   // ALARM on?
                draw_glyph(219, 5, &glyph_bell, al[color_scheme], 2);
            }
        }
        else if (gpio == 4)
        {
            char str[3];
            for (int i = 0, val = 0; i < gpio; i++)
            {
                val = gpiod_line_get_value(gpio_pin[i]);
                if (val < 0)          // error
                    strcpy(str, "??");
                else if (HIGH == val) // 1 if button pressed
                    sprintf(str, "%02d", gpio_set[1][i]);
                else                  // 0 if not pressed
                    strcpy(str, "--");
                draw_fstr(i * 60, 280, str, fg1[color_scheme], 2);
            }
        }
        usleep(200);
    }

    if (gpio)
    {
        // assumes gpio is 2 or 4 when set!
        for (int i = 0; i < gpio; i++)
        {
            // Disable GPIO pins
            gpiod_line_release(gpio_pin[i]);
        }
        gpiod_chip_close(chip);
    }
    // Disconnect framebuffer
    fb_close();
    return 0;
}
