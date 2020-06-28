/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stage4.h"
#include "stage4_events.h"

#include "global.h"
#include "stage.h"
#include "stageutils.h"
#include "util/glm.h"
#include "resource/model.h"
#include "portrait.h"

PRAGMA(message "Remove when this stage is modernized")
DIAGNOSTIC(ignored "-Wdeprecated-declarations")

/*
 *  See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct Stage4DrawData stage4_draw_data;

struct stage4_spells_s stage4_spells = {
	.mid = {
		.gate_of_walachia = {
			{ 0,  1,  2,  3}, AT_Spellcard, "Bloodless “Gate of Walachia”", 50, 44000,
			kurumi_slaveburst, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
		.dry_fountain = {
			{ 4,  5, -1, -1}, AT_Spellcard, "Bloodless “Dry Fountain”", 50, 44000,
			kurumi_redspike, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
		.red_spike = {
			{-1, -1,  6,  7}, AT_Spellcard, "Bloodless “Red Spike”", 50, 46000,
			kurumi_redspike, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
	},

	.boss = {
		.animate_wall = {
			{ 8,  9, -1, -1}, AT_Spellcard, "Limit “Animate Wall”", 60, 50000,
			kurumi_aniwall, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
		.demon_wall = {
			{-1, -1, 10, 11}, AT_Spellcard, "Summoning “Demon Wall”", 60, 55000,
			kurumi_aniwall, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
		.blow_the_walls = {
			{12, 13, 14, 15}, AT_Spellcard, "Power Sign “Blow the Walls”", 60, 55000,
			kurumi_blowwall, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
		.bloody_danmaku = {
			{18, 19, 16, 17}, AT_Spellcard, "Predation “Vampiric Vapor”", 80, 60000,
			kurumi_danmaku, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
	},

	.extra.vlads_army = {
		{ 0,  1,  2,  3}, AT_ExtraSpell, "Blood Magic “Vlad’s Army”", 60, 50000,
		kurumi_extra, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
	},
};

static bool stage4_fog(Framebuffer *fb) {
	float f = 0;
/*	int redtime = 5100 + STAGE4_MIDBOSS_MUSIC_TIME;

	if(global.timer > redtime) {
		float v = (global.timer-redtime)*0.0005;
		f =  v < 0.1 ? v : 0.1;
	}*/

	r_shader("zbuf_fog");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4("fog_color", 9.0*f+0.1, 0.0, 0.1-f, 1.0);
	r_uniform_float("start", 0.4);
	r_uniform_float("end", 1);
	r_uniform_float("exponent", 50.0);
	r_uniform_float("sphereness", 0);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();

	return true;
}


static uint stage4_lake_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 0, 0};
	return single3dpos(s3d, pos, maxrange, p);
}

static void stage4_lake_draw(vec3 pos) {
	vec3 light_pos[] = {
		{0, stage_3d_context.cam.pos[1]+3, stage_3d_context.cam.pos[2]-0.8},
		{0, 25, 3}
	};

	mat4 camera_trans;
	glm_mat4_identity(camera_trans);
	camera3d_apply_transforms(&stage_3d_context.cam, camera_trans);

	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2]);
	r_shader("pbr");
	//r_uniform_vec3_array("light_positions[0]", 0, 1, &stage_3d_context.cx);

	vec3 light_colors[] = {
		{1, 22, 22},
		{4, 20, 22},
	};

	vec3 cam_light_positions[2];
	glm_mat4_mulv3(camera_trans, light_pos[0], 1, cam_light_positions[0]);
	glm_mat4_mulv3(camera_trans, light_pos[1], 1, cam_light_positions[1]);

		
	r_uniform_vec3_array("light_positions[0]", 0, ARRAY_SIZE(cam_light_positions), cam_light_positions);
	r_uniform_vec3_array("light_colors[0]", 0, ARRAY_SIZE(light_colors), light_colors);
	r_uniform_int("light_count", 2);
	
	r_uniform_float("metallic", 0);
	r_uniform_sampler("tex", "stage4/ground_diffuse");
	r_uniform_sampler("roughness_map", "stage4/ground_roughness");
	r_uniform_sampler("normal_map", "stage4/ground_normal");
	r_uniform_sampler("ambient_map", "stage4/ground_ambient");
	r_uniform_vec3("ambient_color", 1, 1, 1);


	r_draw_model("stage4/ground");
	
	r_uniform_float("metallic", 0);
	r_uniform_sampler("tex", "stage4/mansion_diffuse");
	r_uniform_sampler("roughness_map", "stage4/mansion_roughness");
	r_uniform_sampler("normal_map", "stage4/mansion_normal");
	r_uniform_sampler("ambient_map", "stage4/mansion_ambient");

	r_draw_model("stage4/mansion");
	r_mat_mv_pop();
	r_shader_standard();
}

static uint stage4_corridor_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 25, 3};
	vec3 r = {0, 10, 0};

	uint num = linear3dpos(s3d, pos, maxrange, p, r);

	for(uint i = 0; i < num; ++i) {
		if(s3d->pos_buffer[i][1] < p[1]) {
			s3d->pos_buffer[i][1] = -9000;
		}
	}

	return num;
}

static void stage4_corridor_draw(vec3 pos) {
	real xoff = 3;
	real zoff = 1.3;
	vec3 light_pos[] = {
		{-xoff, pos[1], pos[2]+zoff},
		{xoff, pos[1], pos[2]+zoff},
		{-xoff, pos[1]-10, pos[2]+zoff},
		{xoff, pos[1]-10, pos[2]+zoff},
		{-xoff, pos[1]+10, pos[2]+zoff},
		{xoff, pos[1]+10, pos[2]+zoff},
	};

	mat4 camera_trans;
	glm_mat4_identity(camera_trans);
	camera3d_apply_transforms(&stage_3d_context.cam, camera_trans);


	vec3 light_colors[] = {
		{1, 20, 20},
		{1, 20, 20},
		{1, 20, 20},
		{1, 20, 20},
		{1, 20, 20},
		{1, 20, 20},
	};

	

	vec3 cam_light_positions[ARRAY_SIZE(light_pos)];
	for(int i = 0; i < ARRAY_SIZE(light_pos); i++) {
		glm_mat4_mulv3(camera_trans, light_pos[i], 1, cam_light_positions[i]);

		real t = global.frames*0.02;
		real mod1 = cos(13095434*light_pos[i][1]);
		real mod2 = sin(1242435*light_pos[i][0]*light_pos[i][1]);

		double f = (sin((1+mod1)*t) + sin((2.35+mod2)*t+mod1) + sin((3.1257+mod1*mod2)*t+mod2))/3;
		glm_vec3_scale(light_colors[i],0.6+0.4*f, light_colors[i]);
	}


	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2]);
	//r_mat_mv_rotate(pos[1]/2000, 0, 1, 0);
	r_shader("pbr");

	r_uniform_vec3_array("light_positions[0]", 0, ARRAY_SIZE(cam_light_positions), cam_light_positions);
	r_uniform_vec3_array("light_colors[0]", 0, ARRAY_SIZE(light_colors), light_colors);
	r_uniform_int("light_count", ARRAY_SIZE(light_pos));


	r_uniform_float("metallic", 0);
	r_uniform_sampler("tex", "stage4/corridor_diffuse");
	r_uniform_sampler("roughness_map", "stage4/corridor_roughness");
	r_uniform_sampler("normal_map", "stage4/corridor_normal");
	r_uniform_sampler("ambient_map", "stage4/corridor_ambient");
	r_uniform_vec3_rgb("ambient_color", &stage4_draw_data.ambient_color);

	r_draw_model("stage4/corridor");
	r_mat_mv_pop();
}



TASK(upgrade_stage_3d, NO_ARGS) {
	for(;;) {
		YIELD;
		stage3d_update(&stage_3d_context);
	}
}

TASK(animate_bg_fullstage, NO_ARGS) {
	stage_3d_context.cam.pos[2] = -1.8;
	stage_3d_context.cam.pos[1] = -30;
	stage_3d_context.crot[0] = 80;
	
	stage_3d_context.cam.vel[1] = 0.017;
	stage_3d_context.cam.vel[2] = 0.008;

	WAIT(400);
	for(int i = 0; i < 200; i++) {
		YIELD;
		fapproach_p(&stage_3d_context.cam.vel[1], 0.013f, 0.0001f);
		fapproach_p(&stage_3d_context.cam.vel[2], 0.0f, 0.0001f);
	}
	WAIT(600);
	for(int i = 0; i < 200; i++) {
		YIELD;
		fapproach_p(&stage_3d_context.cam.vel[2], 0.003f, 0.0002f);
	}
	WAIT(1000);
	for(int i = 0; i < 400; i++) {
		YIELD;
		fapproach_p(&stage_3d_context.cam.vel[2], -0.003f, 0.0002f);
	}
	for(;;) {
		YIELD;
		fapproach_p(&stage_3d_context.cam.vel[1], 0.05f, 0.001f);
		fapproach_p(&stage_3d_context.cam.vel[2], 0.0f, 0.001f);
	}
}

static void stage4_start(void) {
	stage3d_init(&stage_3d_context, 16);
	memset(&stage4_draw_data, 0, sizeof(stage4_draw_data));
	stage4_draw_data.ambient_color = *RGB(0.1,0.1,0.1);
	INVOKE_TASK(upgrade_stage_3d);
	INVOKE_TASK(animate_bg_fullstage);
}

static void stage4_preload(void) {
	portrait_preload_base_sprite("kurumi", NULL, RESF_DEFAULT);
	portrait_preload_face_sprite("kurumi", "normal", RESF_DEFAULT);
	preload_resources(RES_BGM, RESF_OPTIONAL, "stage4", "stage4boss", NULL);
	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"stage4/kurumibg1",
		"stage4/kurumibg2",
		"stage4/ground_ambient",
		"stage4/ground_diffuse",
		"stage4/ground_roughness",
		"stage4/ground_normal",
		"stage4/mansion_ambient",
		"stage4/mansion_diffuse",
		"stage4/mansion_roughness",
		"stage4/mansion_normal",
		"stage4/corridor_ambient",
		"stage4/corridor_diffuse",
		"stage4/corridor_roughness",
		"stage4/corridor_normal",
	NULL);
	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"stage6/scythe", // Stage 6 is intentional
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"pbr",
		"zbuf_fog",
		"sprite_negative",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_OPTIONAL,
		"lasers/accelerated",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"boss/kurumi",
	NULL);
	preload_resources(RES_MODEL, RESF_DEFAULT,
		"stage4/mansion",
		"stage4/ground",
		"stage4/corridor",
	NULL);
	preload_resources(RES_TEXTURE, RESF_OPTIONAL,
		"part/sinewave",
	NULL);
	preload_resources(RES_SFX, RESF_OPTIONAL,
		"laser1",
		"boom",
		"warp",
	NULL);

	// XXX: Special case for spell practice of the god damn extra spell, because it always needs a special case.
	// TODO: Maybe add spell-specific preloads instead of putting everything into the stage one?
	enemies_preload();
}

static void stage4_end(void) {
	stage3d_shutdown(&stage_3d_context);
}

static void stage4_draw(void) {
	r_mat_proj_perspective(STAGE3D_DEFAULT_FOVY, VIEWPORT_W/(double)VIEWPORT_H, 1, 300);

	Stage3DSegment segs[] = {
		{ stage4_lake_draw, stage4_lake_pos },
		{ stage4_corridor_draw, stage4_corridor_pos },
	};

	stage3d_draw(&stage_3d_context, 400, ARRAY_SIZE(segs), segs);
}

static void stage4_update(void) {
}

static void stage4_spellpractice_start(void) {
	stage4_start();
	skip_background_anim(stage4_update, 3200, &global.frames, NULL);

	global.boss = stage4_spawn_kurumi(BOSS_DEFAULT_SPAWN_POS);
	boss_add_attack_from_info(global.boss, global.stage->spell, true);
	boss_start_attack(global.boss, global.boss->attacks);

	stage_start_bgm("stage4boss");
}

static void stage4_spellpractice_events(void) {
}

void stage4_skip(int t) {
	skip_background_anim(stage4_update, t, &global.timer, &global.frames);
	audio_bgm_seek_realtime(global.timer / (double)FPS);
}

ShaderRule stage4_shaders[] = { stage4_fog, NULL };

StageProcs stage4_procs = {
	.begin = stage4_start,
	.preload = stage4_preload,
	.end = stage4_end,
	.draw = stage4_draw,
	.update = stage4_update,
	.event = stage4_events,
	.shader_rules = stage4_shaders,
	.spellpractice_procs = &stage4_spell_procs,
};

StageProcs stage4_spell_procs = {
	.begin = stage4_spellpractice_start,
	.preload = stage4_preload,
	.end = stage4_end,
	.draw = stage4_draw,
	.update = stage4_update,
	.event = stage4_spellpractice_events,
	.shader_rules = stage4_shaders,
};
