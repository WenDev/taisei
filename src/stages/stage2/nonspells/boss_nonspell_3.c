/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"
#include "../hina.h"

#include "common_tasks.h"
#include "global.h"

// Danmakufu coords -> Taisei coords
#define SPACE_SCALE 1.25

TASK(TColumn, { BoxedBoss boss; cmplx dir; real dir_sign; real odd; int num; real rank; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	int delay = 20;
	real dist = 64 * SPACE_SCALE;

	// NOTE: bullet flash warning dummied out, replaced with simple wait
	WAIT(delay);

	cmplx dir = ARGS.dir;           // this plays the role of "ang" in the original code, but in complex/vector form
	real dir_sign = ARGS.dir_sign;  // this is "dir" in the original code
	real odd = ARGS.odd;
	int num = ARGS.num;
	real rank = ARGS.rank;
	cmplx shot_origin = boss->pos + dist * dir;

	for(int i = 0; i < num; ++i) {
		real spd = SPACE_SCALE * rank * (3 + (1.5 * i) / num);
		// this is like the `ang + 45*odd*dir + rand(-1,1)` in original code, but again expressed with complex numbers
		cmplx aim = dir * cdir(DEG2RAD * (45 * odd * dir_sign + rng_sreal()));

		PROJECTILE(
			.proto = pp_card,
			.color = RGB(1, 0, 0),
			// NOTE: original code seems to limit this to spd
			// This is probably better approximated with move_asymptotic, but it's not a direct translation
			// .move = move_accelerated(1.25 * spd * aim, -spd/120 * aim),
			.move = move_asymptotic_simple(spd * aim, 2),
			.pos = shot_origin,
		);

		WAIT(5);
	}
}

TASK(XPattern, { BoxedBoss boss; real dir_sign; int num2; int num3; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	real rank = difficulty_value(0.4, 0.65, 0.85, 1);
	int rof = lround(3 / rank);
	int num = 180 * rank;
	int num2 = ARGS.num2 * rank;
	int num3 = ARGS.num3 * rank;
	real odd = 1;
	real dir_sign = ARGS.dir_sign;
	cmplx rot = cnormalize(global.plr.pos - boss->pos) * cdir(M_PI/2 * -dir_sign);

	for(int i = 0; i < num2; ++i) {
		for(int j = 0; j < 3; ++j) {
			INVOKE_SUBTASK(TColumn,
				.boss = ENT_BOX(boss),
				.dir = rot * cdir(DEG2RAD * (dir_sign * i * 720 / (num - 1.0) - (30 + 60 * j) * odd * dir_sign)),
				.dir_sign = dir_sign,
				.odd = odd,
				.num = num3,
				.rank = rank
			);
		}

		odd = -odd;
		WAIT(rof);
	}
}

TASK(TShoot, { BoxedBoss boss; real ang; real ang_s; real dist; real spd_inc; real dir_sign; int t; int time; int num; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	real ang = ARGS.ang;
	real ang_s = ARGS.ang_s;
	real dist = ARGS.dist;
	real spd_inc = ARGS.spd_inc;
	real dir_sign = ARGS.dir_sign;
	int t = ARGS.t;
	int time = ARGS.time;
	int num = ARGS.num;

	for(int i = 0; i < num; ++i) {
		real ang2 = ang - ang_s * i * dir_sign;
		cmplx shot_origin = boss->pos + cdir(ang2) * dist;
		real ang_shoot = ang2 - M_PI/2 * dir_sign;
		real spd = SPACE_SCALE * (1.5 - 0.5 * (i / num)) + (spd_inc * t) / time;

		PROJECTILE(
			.proto = (t > 0.6 * time) ? pp_crystal : pp_card,
			.color = RGB(t / (real)time, 0, 1),
			.pos = shot_origin,
			.move = move_linear(spd * cdir(ang_shoot)),
		);

		WAIT(3);
	}
}

TASK(SpinPattern, { BoxedBoss boss; real dir_sign; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	int rof = difficulty_value(12, 5, 4, 3);
	real spd_inc = difficulty_value(1, 1.5, 2, 2.5) * SPACE_SCALE;

	int time = 240;
	int num = 3;
	real dir_sign = ARGS.dir_sign;
	real ang = M_PI/2 * (1 - dir_sign);
	real ang_d = dir_sign * rof * (2*M_TAU - M_PI/2) / time;
	real ang_s = M_TAU/3;
	real ang_s_d = -(M_TAU/3) / (time / (real)rof);
	real dist = 16 * SPACE_SCALE;
	real dist_d = 256.0 / time * SPACE_SCALE;

	// NOTE: bullet flash warning dummied out, replaced with simple wait
	WAIT(20);

	for(int t = 0; t < time;) {
		INVOKE_SUBTASK(TShoot,
			.boss = ENT_BOX(boss),
			.ang = ang,
			.ang_s = ang_s,
			.dist = dist,
			.spd_inc = spd_inc,
			.dir_sign = dir_sign,
			.t = t,
			.time = time,
			.num = num
		);

		ang_s += ang_s_d;
		dist += dist_d;
		ang += ang_d;

		t += WAIT(rof);
	}
}

static cmplx random_boss_pos(void) {
	return VIEWPORT_W/2 + SPACE_SCALE * (rng_sreal() * 32 + I * rng_range(132, 144));
}

DEFINE_EXTERN_TASK(stage2_boss_nonspell_3) {
	Boss *boss = INIT_BOSS_ATTACK();
	boss->move = move_towards(VIEWPORT_W/2.0 + 100.0*I, 0.02);
	BEGIN_BOSS_ATTACK();

	real dir_sign = rng_sign();

	boss->move.attraction_point = random_boss_pos();
	WAIT(60);
	// NOTE: animation dummied out
	WAIT(20);
	INVOKE_SUBTASK(SpinPattern, ENT_BOX(boss), dir_sign);
	WAIT(30);
	INVOKE_SUBTASK(SpinPattern, ENT_BOX(boss), dir_sign);
	WAIT(30);
	INVOKE_SUBTASK(SpinPattern, ENT_BOX(boss), dir_sign);
	WAIT(210);
	INVOKE_SUBTASK(XPattern,
		.boss = ENT_BOX(boss),
		.dir_sign = dir_sign,
		.num2 = 42,
		.num3 = 6
	);

	for(;;) {
		dir_sign = -dir_sign;

		boss->move.attraction_point = random_boss_pos();
		WAIT(60);
		// NOTE: animation dummied out
		WAIT(20);

		INVOKE_SUBTASK(XPattern,
			.boss = ENT_BOX(boss),
			.dir_sign = dir_sign,
			.num2 = 42,
			.num3 = 6
		);

		INVOKE_SUBTASK(SpinPattern, ENT_BOX(boss), dir_sign);
		WAIT(30);
		INVOKE_SUBTASK(SpinPattern, ENT_BOX(boss), dir_sign);
		WAIT(30);
		INVOKE_SUBTASK(SpinPattern, ENT_BOX(boss), dir_sign);
		WAIT(180);

		INVOKE_SUBTASK(XPattern,
			.boss = ENT_BOX(boss),
			.dir_sign = dir_sign,
			.num2 = 42,
			.num3 = 9
		);
	}
}
