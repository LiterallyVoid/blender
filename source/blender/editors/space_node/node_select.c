/**
 * $Id:
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The Original Code is Copyright (C) 2008 Blender Foundation.
 * All rights reserved.
 *
 * 
 * Contributor(s): Blender Foundation, Nathan Letwory
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <stdio.h>

#include "DNA_node_types.h"
#include "DNA_scene_types.h"
#include "DNA_screen_types.h"
#include "DNA_space_types.h"

#include "BKE_context.h"
#include "BKE_node.h"
#include "BKE_global.h"

#include "BLI_rect.h"

#include "ED_space_api.h"
#include "ED_screen.h"
#include "ED_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "WM_api.h"
#include "WM_types.h"

#include "UI_view2d.h"
 
#include "node_intern.h"
 
static void node_mouse_select(SpaceNode *snode, ARegion *ar, short *mval, short extend)
{
	bNode *node;
	float mx, my;
	
	mx= (float)mval[0];
	my= (float)mval[1];
	
	UI_view2d_region_to_view(&ar->v2d, mval[0], mval[1], &mx, &my);
	
	for(next_node(snode->edittree); (node=next_node(NULL));) {
		
		/* first check for the headers or scaling widget */
		/* XXX if(node->flag & NODE_HIDDEN) {
			if(do_header_hidden_node(snode, node, mx, my))
				return 1;
		}
		else {
			if(do_header_node(snode, node, mx, my))
				return 1;
		}*/
		
		/* node body */
		if(BLI_in_rctf(&node->totr, mx, my))
			break;
	}
	if(node) {
		if((extend & KM_SHIFT)==0)
			node_deselectall(snode, 0);
		
		if(extend & KM_SHIFT) {
			if(node->flag & SELECT)
				node->flag &= ~SELECT;
			else
				node->flag |= SELECT;
		}
		else
			node->flag |= SELECT;
		
		node_set_active(snode, node);
		
		/* viewer linking */
		if(extend & KM_CTRL)
			;//	node_link_viewer(snode, node);
		
		//std_rmouse_transform(node_transform_ext);	/* does undo push for select */
		ED_region_tag_redraw(ar);
	}
}

static int node_select_exec(bContext *C, wmOperator *op)
{
	SpaceNode *snode= (SpaceNode*)CTX_wm_space_data(C);
	ARegion *ar= CTX_wm_region(C);
	int select_type;
	short mval[2];
	short extend;

	select_type = RNA_enum_get(op->ptr, "select_type");
	
	switch (select_type) {
		case NODE_SELECT_MOUSE:
			mval[0] = RNA_int_get(op->ptr, "mx");
			mval[1] = RNA_int_get(op->ptr, "my");
			extend = RNA_int_get(op->ptr, "extend");
			node_mouse_select(snode, ar, mval, extend);
			break;
	}
	return OPERATOR_FINISHED;
}

static int node_select_invoke(bContext *C, wmOperator *op, wmEvent *event)
{
	ARegion *ar= CTX_wm_region(C);
	short mval[2];	
	
	mval[0]= event->x - ar->winrct.xmin;
	mval[1]= event->y - ar->winrct.ymin;
	
	RNA_int_set(op->ptr, "mx", mval[0]);
	RNA_int_set(op->ptr, "my", mval[1]);

	return node_select_exec(C,op);
}

static int node_extend_select_invoke(bContext *C, wmOperator *op, wmEvent *event)
{
	RNA_int_set(op->ptr, "extend", KM_SHIFT);

	return node_select_invoke(C, op, event);
}

/* operators */

static EnumPropertyItem prop_select_items[] = {
	{NODE_SELECT_MOUSE, "NORMAL", "Normal Select", "Select using the mouse"},
	{0, NULL, NULL, NULL}};

void NODE_OT_extend_select(wmOperatorType *ot)
{
	PropertyRNA *prop;
	
	/* identifiers */
	ot->name= "Activate/Select (Shift)";
	ot->idname= "NODE_OT_extend_select";
	
	/* api callbacks */
	ot->invoke= node_extend_select_invoke;
	ot->poll= ED_operator_node_active;
	
	prop = RNA_def_property(ot->srna, "select_type", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, prop_select_items);
	
	prop = RNA_def_property(ot->srna, "mx", PROP_INT, PROP_NONE);
	prop = RNA_def_property(ot->srna, "my", PROP_INT, PROP_NONE);
	prop = RNA_def_property(ot->srna, "extend", PROP_INT, PROP_NONE);
}

void NODE_OT_select(wmOperatorType *ot)
{
	PropertyRNA *prop;
	
	/* identifiers */
	ot->name= "Activate/Select";
	ot->idname= "NODE_OT_select";
	
	/* api callbacks */
	ot->invoke= node_select_invoke;
	ot->poll= ED_operator_node_active;
	
	prop = RNA_def_property(ot->srna, "select_type", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, prop_select_items);
	
	prop = RNA_def_property(ot->srna, "mx", PROP_INT, PROP_NONE);
	prop = RNA_def_property(ot->srna, "my", PROP_INT, PROP_NONE);
	prop = RNA_def_property(ot->srna, "extend", PROP_INT, PROP_NONE);
}

/* ****** Border Select ****** */

static EnumPropertyItem prop_select_types[] = {
	{NODE_EXCLUSIVE, "EXCLUSIVE", "Exclusive", ""}, /* right mouse */
	{NODE_EXTEND, "EXTEND", "Extend", ""}, /* left mouse */
	{0, NULL, NULL, NULL}
};

static int node_borderselect_exec(bContext *C, wmOperator *op)
{
	SpaceNode *snode= (SpaceNode*)CTX_wm_space_data(C);
	ARegion *ar= CTX_wm_region(C);
	bNode *node;
	rcti rect;
	rctf rectf;
	short val;
	
	val= RNA_int_get(op->ptr, "event_type");
	
	rect.xmin= RNA_int_get(op->ptr, "xmin");
	rect.ymin= RNA_int_get(op->ptr, "ymin");
	UI_view2d_region_to_view(&ar->v2d, rect.xmin, rect.ymin, &rectf.xmin, &rectf.ymin);
	
	rect.xmax= RNA_int_get(op->ptr, "xmax");
	rect.ymax= RNA_int_get(op->ptr, "ymax");
	UI_view2d_region_to_view(&ar->v2d, rect.xmax, rect.ymax, &rectf.xmax, &rectf.ymax);
	
	for(node= snode->edittree->nodes.first; node; node= node->next) {
		if(BLI_isect_rctf(&rectf, &node->totr, NULL)) {
			if(val==NODE_EXTEND)
				node->flag |= SELECT;
			else
				node->flag &= ~SELECT;
		}
	}
	
	return OPERATOR_FINISHED;
}

void NODE_OT_border_select(wmOperatorType *ot)
{
	PropertyRNA *prop;
	
	/* identifiers */
	ot->name= "Border Select";
	ot->idname= "NODE_OT_border_select";
	
	/* api callbacks */
	ot->invoke= WM_border_select_invoke;
	ot->exec= node_borderselect_exec;
	ot->modal= WM_border_select_modal;
	
	ot->poll= ED_operator_node_active;
	
	/* rna */
	RNA_def_property(ot->srna, "event_type", PROP_INT, PROP_NONE);
	RNA_def_property(ot->srna, "xmin", PROP_INT, PROP_NONE);
	RNA_def_property(ot->srna, "xmax", PROP_INT, PROP_NONE);
	RNA_def_property(ot->srna, "ymin", PROP_INT, PROP_NONE);
	RNA_def_property(ot->srna, "ymax", PROP_INT, PROP_NONE);

	prop = RNA_def_property(ot->srna, "type", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, prop_select_types);
}