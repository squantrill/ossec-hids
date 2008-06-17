/* @(#) $Id$ */

/* Copyright (C) 2004-2008 Third Brigade, Inc.
 * All right reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 3) as published by the FSF - Free Software
 * Foundation
 */

 
#ifndef _AR__H
#define _AR__H

#include "config/config.h"
#include "config/active-response.h"
#include "list_op.h"


/** void AR_Init()
 * Initializing active response.
  */
void AR_Init();

/** int AR_ReadConfig(int test_config, char *cfgfile)
 * Reads active response configuration and write them
 * to the appropriate lists.
 */
int AR_ReadConfig(int test_config, char *cfgfile);
     

/* Active response commands */
OSList *ar_commands;

/* Active response information */
OSList *active_responses;


#endif
