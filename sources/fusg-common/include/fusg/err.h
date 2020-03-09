/*
 * err.h
 *
 *  Created on: 6 Mar 2020
 *      Author: homac
 */

#ifndef FUSG_ERR_H_
#define FUSG_ERR_H_

enum {
	ERR_NONE = 0,
	ERR_USAGE,
	ERR_CONF,
	ERR_DB,
	ERR_AUPARSE,
	ERR_UNKNOWN = 255,
};

#endif /* FUSG_ERR_H_ */
