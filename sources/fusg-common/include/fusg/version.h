/*
 * version.h
 *
 *  Created on: 7 Mar 2020
 *      Author: homac
 */

#ifndef FUSG_VERSION_H_
#define FUSG_VERSION_H_


#define FUSG_VER_MAJOR 1
#define FUSG_VER_MINOR 0
#define FUSG_VER_BUILD 0
#define FUSG_VER_STAGE alpha

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define FUSG_VERSION FUSG_VER_MAJOR ## FUSG_VER_MINOR


#ifndef FUSG_VER_STAGE
#  define FUSG_VER_STR TOSTRING(FUSG_VER_MAJOR.FUSG_VER_MINOR.FUSG_VER_BUILD)
#else
#  define FUSG_VER_STR TOSTRING(FUSG_VER_MAJOR.FUSG_VER_MINOR.FUSG_VER_BUILD-FUSG_VER_STAGE)
#endif

#endif /* FUSG_VERSION_H_ */
