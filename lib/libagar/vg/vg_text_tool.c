/*
 * Copyright (c) 2008 Hypertriton, Inc. <http://hypertriton.com/>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Text tool.
 */

#include <core/core.h>
#include <gui/widget.h>
#include <gui/primitive.h>
#include <gui/textbox.h>

#include "vg.h"
#include "vg_view.h"
#include "icons.h"

typedef struct vg_text_tool {
	VG_Tool _inherit;
	VG_Text *vtIns;			/* Text being inserted */
	char text[VG_TEXT_MAX];
} VG_TextTool;

static void
Init(void *p)
{
	VG_TextTool *t = p;

	t->vtIns = NULL;
	Strlcpy(t->text, "<text>", sizeof(t->text));
}

static int
MouseButtonDown(void *p, VG_Vector vPos, int button)
{
	VG_TextTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG *vg = vv->vg;
	VG_Point *p1, *p2;

	switch (button) {
	case SDL_BUTTON_LEFT:
		if (t->vtIns == NULL) {
			if (!(p1 = VG_NearestPoint(vv, vPos, NULL))) {
				p1 = VG_PointNew(vg->root, vPos);
			}
			p2 = VG_PointNew(vg->root, vPos);
			t->vtIns = VG_TextNew(vg->root, p1, p2);
			VG_TextPrintf(t->vtIns, "%s", t->text);
			if (VGTOOL(t)->p != NULL)
				VG_TextSubstObject(t->vtIns, VGTOOL(t)->p);
		} else {
			if ((p2 = VG_NearestPoint(vv, vPos,
			    t->vtIns->p2))) {
				VG_DelRef(t->vtIns, t->vtIns->p2);
				VG_Delete(t->vtIns->p2);
				t->vtIns->p2 = p2;
				VG_AddRef(t->vtIns, p2);
			} else {
				VG_SetPosition(t->vtIns->p2, vPos);
			}
			t->vtIns = NULL;
		}
		return (1);
	case SDL_BUTTON_RIGHT:
		if (t->vtIns != NULL) {
			VG_Delete(t->vtIns);
			t->vtIns = NULL;
		}
		return (1);
	default:
		return (0);
	}
}

static void
PostDraw(void *p, VG_View *vv)
{
	VG_TextTool *t = p;
	int x, y;

	VG_GetViewCoords(vv, VGTOOL(t)->vCursor, &x,&y);
	AG_DrawCircle(vv, x,y, 3, VG_MapColorRGB(vv->vg->selectionColor));
}

static int
MouseMotion(void *p, VG_Vector vPos, VG_Vector vRel, int b)
{
	VG_TextTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Point *pEx;
	VG_Vector pos;
	float theta, rad;
	
	if (t->vtIns != NULL) {
		pEx = t->vtIns->p1;
		pos = VG_Pos(pEx);
		theta = VG_Atan2(vPos.y - pos.y,
		                 vPos.x - pos.x);
		rad = VG_Hypot(vPos.x - pos.x,
		               vPos.y - pos.y);
		if ((pEx = VG_NearestPoint(vv, vPos, t->vtIns->p2))) {
			VG_Status(vv, _("End baseline at Point%u"),
			    VGNODE(pEx)->handle);
		} else {
			VG_Status(vv,
			    _("End baseline at %.2f,%.2f "
			      "(%.2f|%.2f\xc2\xb0)"),
			    vPos.x, vPos.y, rad, VG_Degrees(theta));
		}
		VG_SetPosition(t->vtIns->p2, vPos);
	} else {
		if ((pEx = VG_NearestPoint(vv, vPos, NULL))) {
			VG_Status(vv, _("Start baseline at Point%u"),
			    VGNODE(pEx)->handle);
		} else {
			VG_Status(vv, _("Start baseline at %.2f,%.2f"), vPos.x,
			    vPos.y);
		}
	}
	return (0);
}

static void *
Edit(void *p, VG_View *vv)
{
	VG_TextTool *t = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);
	AG_Textbox *tb;

	AG_LabelNew(box, 0, _("Text: "));
	tb = AG_TextboxNew(box, AG_TEXTBOX_MULTILINE|AG_TEXTBOX_HFILL, NULL);
	AG_TextboxBindUTF8(tb, t->text, sizeof(t->text));
	return (box);
}

VG_ToolOps vgTextTool = {
	N_("Text"),
	N_("Insert text entity."),
	&vgIconText,
	sizeof(VG_TextTool),
	0,
	Init,
	NULL,			/* destroy */
	Edit,
	NULL,			/* predraw */
	PostDraw,
	NULL,			/* selected */
	NULL,			/* deselected */
	MouseMotion,
	MouseButtonDown,
	NULL,			/* mousebuttonup */
	NULL,			/* keydown */
	NULL			/* keyup */
};
