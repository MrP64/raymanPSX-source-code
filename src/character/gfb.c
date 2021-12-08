/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "gfb.h"

#include "../mem.h"
#include "../archive.h"
#include "../stage.h"
#include "../main.h"

#include "speaker.h"


//gfb character structure
enum
{
	gfb_ArcMain_BopLeft,
	gfb_ArcMain_BopRight,
	gfb_ArcMain_Cry,
	
	gfb_Arc_Max,
};

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main;
	IO_Data arc_ptr[gfb_Arc_Max];
	
	Gfx_Tex tex;
	u8 frame, tex_id;
	
	//Speaker
	Speaker speaker;
	
} Char_gfb;

//gfb character definitions
static const CharFrame char_gfb_frame[] = {
	{gfb_ArcMain_BopLeft, {  0,   0,  74, 103}, { 40,  73}}, //0 bop left 1
	{gfb_ArcMain_BopLeft, { 74,   0,  73, 102}, { 39,  73}}, //1 bop left 2
	{gfb_ArcMain_BopLeft, {147,   0,  73, 102}, { 39,  73}}, //2 bop left 3
	{gfb_ArcMain_BopLeft, {  0, 103,  73, 103}, { 39,  74}}, //3 bop left 4
	{gfb_ArcMain_BopLeft, { 73, 102,  82, 105}, { 43,  76}}, //4 bop left 5
	{gfb_ArcMain_BopLeft, {155, 102,  81, 105}, { 43,  76}}, //5 bop left 6
	
	{gfb_ArcMain_BopRight, {  0,   0,  81, 103}, { 43,  74}}, //6 bop right 1
	{gfb_ArcMain_BopRight, { 81,   0,  81, 103}, { 43,  74}}, //7 bop right 2
	{gfb_ArcMain_BopRight, {162,   0,  80, 103}, { 42,  74}}, //8 bop right 3
	{gfb_ArcMain_BopRight, {  0, 103,  79, 103}, { 41,  74}}, //9 bop right 4
	{gfb_ArcMain_BopRight, { 79, 103,  73, 105}, { 35,  76}}, //10 bop right 5
	{gfb_ArcMain_BopRight, {152, 103,  74, 104}, { 35,  75}}, //11 bop right 6
	
	{gfb_ArcMain_Cry, {  0,   0,  73, 101}, { 37,  73}}, //12 cry
	{gfb_ArcMain_Cry, { 73,   0,  73, 101}, { 37,  73}}, //13 cry
};

static const Animation char_gfb_anim[CharAnim_Max] = {
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Left}},                           //CharAnim_Idle
	{1, (const u8[]){ 0,  0,  1,  1,  2,  2,  3,  4,  4,  5, ASCR_BACK, 1}}, //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Left}},                           //CharAnim_LeftAlt
	{2, (const u8[]){12, 13, ASCR_REPEAT}},                                  //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Left}},                           //CharAnim_DownAlt
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Left}},                           //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Left}},                           //CharAnim_UpAlt
	{1, (const u8[]){ 6,  6,  7,  7,  8,  8,  9, 10, 10, 11, ASCR_BACK, 1}}, //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Left}},                           //CharAnim_RightAlt
};

//gfb character functions
void Char_gfb_SetFrame(void *user, u8 frame)
{
	Char_gfb *this = (Char_gfb*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_gfb_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_gfb_Tick(Character *character)
{
	Char_gfb *this = (Char_gfb*)character;
	
		if (stage.flag & STAGE_FLAG_JUST_STEP)
		{
			//Perform dance
			if ((stage.song_step % stage.gf_speed) == 0)
			{
				//Switch animation
				if (character->animatable.anim == CharAnim_Left)
					character->set_anim(character, CharAnim_Right);
				else
					character->set_anim(character, CharAnim_Left);
				
				//Bump speakers
				Speaker_Bump(&this->speaker);
			}
	}
	
	//Animate and draw
	Animatable_Animate(&character->animatable, (void*)this, Char_gfb_SetFrame);
	Character_Draw(character, &this->tex, &char_gfb_frame[this->frame]);
	
	//Tick speakers
	Speaker_Tick(&this->speaker, character->x, character->y);
}

void Char_gfb_SetAnim(Character *character, u8 anim)
{
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
}

void Char_gfb_Free(Character *character)
{
	Char_gfb *this = (Char_gfb*)character;
	
	//Free art
	Mem_Free(this->arc_main);
}

Character *Char_gfb_New(fixed_t x, fixed_t y)
{
	//Allocate gfb object
	Char_gfb *this = Mem_Alloc(sizeof(Char_gfb));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_gfb_New] Failed to allocate gfb object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_gfb_Tick;
	this->character.set_anim = Char_gfb_SetAnim;
	this->character.free = Char_gfb_Free;
	
	Animatable_Init(&this->character.animatable, char_gfb_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = 0;
	
	this->character.health_i = 1;
	
	this->character.focus_x = FIXED_DEC(16,1);
	this->character.focus_y = FIXED_DEC(-50,1);
	this->character.focus_zoom = FIXED_DEC(13,10);
	
	//Load art
	this->arc_main = IO_Read("\\CHAR\\GF.ARC;1");
	
	const char **pathp = (const char *[]){
		"bopleftb.tim",  //gfb_ArcMain_BopLeft
		"boprightb.tim", //gfb_ArcMain_BopRight
		"cryb.tim",      //gfb_ArcMain_Cry
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find(this->arc_main, *pathp);

	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	//Initialize speaker
	Speaker_Init(&this->speaker);

	
	return (Character*)this;
}
