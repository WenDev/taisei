/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_stages_entities_h
#define IGUARD_stages_entities_h

#include "taisei.h"

#include "stagex/entities.h"

#define ENTITIES_STAGES(X, ...) \
	ENTITIES_STAGEX(X, __VA_ARGS__) \

#endif // IGUARD_stages_entities_h
