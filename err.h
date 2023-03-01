/*
 * This file is part of dfu-discovery.
 *
 * Copyright 2023 ARDUINO SA (http://www.arduino.cc/)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
// The original macros are defined as follows:
# define warnx(...) do {\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(stderr, "\n");\ } while (0)
# define warn(...) do {\
    fprintf(stderr, "%s: ", strerror(errno));\
    warnx(__VA_ARGS__); } while (0)
*/
// No-OP: do not report any warning
# define warnx(...)      do { } while(0)
# define warn(...)       do { } while(0)

# define errx(eval, ...) do {\
    warnx(__VA_ARGS__);\
    exit(eval); } while (0)
# define err(eval, ...) do {\
    warn(__VA_ARGS__);\
    exit(eval); } while (0)
