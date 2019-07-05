/*
 * Copyright (c) 2019, Charles Daniels All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <agar/core.h>
#include <agar/gui.h>

#include <stdlib.h>
#include <string.h>

#include "nocsim.h"

/* work around incorrect SV sizing with AG_Editable */
#define SV_WORKAROUND(parent) \
	AG_WidgetHideAll(\
			AG_LabelNewS(parent, 0,\
		"(SV_WORKAROUND)                                           "))

/* handy macro to check if a string ends with a certain 4 letter character
 * sequence. This is used for checking file extensions. */
#define check_ext(str, ext) (strlen(str) > 4 && !strcmp(str + strlen(str) - 4, ext))

/* Helper macro for use within event handlers, traverses up the VFS tree to
 * grab the top-level driver object. This is needed since AG_ObjectRoot()
 * dosen't always traverse all the way up the tree in one go */
#define get_dri() (AG_Driver*) AG_ObjectFindParent(AG_SELF(), "agDrivers", NULL)

/* handler for "quit" menu item */
void QuitApplication(AG_Event *event) {
	AG_DestroyGraphics();
	AG_Destroy();
	AG_Quit();
}

/* If the debugger is enabled, the "Launch Debugger" menu item will use this
 * to display the LibAgar debugging window */
#ifdef ENABLE_DEBUGGER
void LaunchDebugger(AG_Event *event) {
	AG_Driver* dri;
	AG_Window* db_window;

	/* Fetch the top-level driver object, which is also the parent of the
	 * main window. We will attach the debugger window to this object. */
	dri = (AG_Driver*) get_dri();

	db_window = AG_GuiDebugger(dri);
	AG_WindowShow(db_window);
}
#endif

static void
RenderToSurface(AG_Event *event)
{
    AG_Button *btn = AG_PTR(1);
    AG_Window *winParent = AG_PTR(2), *win;
    AG_Surface *S;

    /* Render the AG_Button to a surface. */
    if ((S = AG_WidgetSurface(btn)) == NULL) {
        AG_TextMsgFromError();
        return;
    }

    /* Display the rendered surface. */
    if ((win = AG_WindowNew(0)) != NULL) {
        AG_WindowSetCaptionS(win, "Rendered surface");
        AG_LabelNew(win, 0, "Surface generated from %s:", AGOBJECT(btn)->name);
        AG_SeparatorNewHoriz(win);
        AG_PixmapFromSurface(win, 0, S);
        AG_SeparatorNewHoriz(win);
        AG_LabelNew(win, 0,
            "Format: %u x %u x %d bpp",
            S->w, S->h,
            S->format.BitsPerPixel);

        AG_WindowAttach(winParent, win);
        AG_WindowShow(win);
    }
}

/* handler for the OK button in the file dialog */
void ExportGraph(AG_Event* event) {

	AG_Graph* g = (AG_Graph*) AG_PTR_NAMED("g");
	AG_Label* statuslabel = (AG_Label*) AG_PTR_NAMED("statuslabel");

	/* the path and filetype are generated by the file-chosen event type */
	char* path = AG_STRING(3);
	AG_FileType* ft = (AG_FileType*) AG_PTR(4);

	/* this is not safe, as it does not do bounds checking */
	char* path_with_ext = malloc(sizeof(char) * (strlen(path) + 4));
	strcpy(path_with_ext, path);

	/* force the widget to redraw, to make sure that the state we pull
	 * out of the surface is current */
	AG_WidgetDraw(g);

	/* copy the current state of the g surface to a new surface object */
	AG_Surface* surf = AG_WidgetSurface(g);

	/* this is an awful way to do this... don't do this */
	if (strcmp(ft->descr, "PNG image") == 0) {
		if (!check_ext(path, ".png")) {
			/* unsafe, does not bounds check */
			strcat(path_with_ext, ".png");
		}
		AG_SurfaceExportPNG(surf, path_with_ext, 0);

	} else if (strcmp(ft->descr, "JPEG image") == 0) {
		if (!check_ext(path, ".jpg")) {
			/* unsafe, does not bounds check */
			strcat(path_with_ext, ".jpg");
		}
		AG_SurfaceExportJPEG(surf, path_with_ext, 80, 0);

	} else {
		if (!check_ext(path, ".bmp")) {
			/* unsafe, does not bounds check */
			strcat(path_with_ext, ".bmp");
		}
		AG_SurfaceExportBMP(surf, path, 0);
		/* AG_SurfaceExportBMP(surf, path); */
	}

	/* clean up the surface object */
	AG_SurfaceFree(surf);

}

void HandleVertexSelection(AG_Event* event) {
	AG_GraphVertex* vtx = AG_PTR(1);
	AG_Driver* dri = get_dri();
	AG_Box* inner;
	nocsim_node* node;

	AG_Box* box = AG_GetPointer(dri, "infobox_p");
	AG_Object* parent = AG_ObjectParent(AGOBJECT(box));
	AG_ObjectDelete(box);
	/* box = AG_BoxNew(parent, AG_BOX_HORIZ, AG_BOX_EXPAND); */
	box = AG_BoxNew(parent, AG_BOX_VERT, AG_BOX_EXPAND);
	AG_SetPointer(dri, "infobox_p", box);

	node = ((nocsim_node*) vtx->userPtr);

	if (vtx->userPtr != NULL) {

		inner = AG_BoxNew(box, AG_BOX_VERT, AG_BOX_FRAME | AG_BOX_HFILL);
		AG_BoxSetLabelS(inner, "node data");

#define prval(label, fmt, ...) \
		AG_TextboxPrintf( \
			AG_TextboxNewS(inner, AG_TEXTBOX_READONLY | AG_TEXTBOX_HFILL, label), \
			fmt, __VA_ARGS__);

		prval("ID", "%s", node->id);
		prval("type", "%s", NOCSIM_NODE_TYPE_TO_STR(node->type));

		prval("node number"   , "%i" , node->node_number);
		prval("type number"   , "%i" , node->type_number);
		prval("row"           , "%i" , node->row);
		prval("col"           , "%i" , node->col);
		prval("behavior"      , "%s" , node->behavior);
		if (node->type == node_PE) {
			prval("flits pending" , "%s" , node->pending->length);
		} else {
			prval("flits pending" , "%s" , "N/A");
		}

		inner = AG_BoxNew(box, AG_BOX_VERT, AG_BOX_FRAME | AG_BOX_HFILL);
		AG_BoxSetLabelS(inner, "performance counters");

		prval("injected", "%li", node->injected);
		prval("routed", "%li", node->routed);

		SV_WORKAROUND(inner);

#undef prval


	} else {
		AG_LabelNewS(box, 0, "node data inaccessible");
	}

	AG_WidgetHide(box);
	AG_WidgetShow(box);

}


/* Export the graph view to a file */
void ExportGraphDialog(AG_Event* event) {

	AG_Graph* g = (AG_Graph*) AG_PTR_NAMED("g");
	AG_Label* statuslabel = (AG_Label*) AG_PTR_NAMED("statuslabel");

	/* create a new window in which to display the file dialog */
	AG_Window* fdwin = AG_WindowNew(0);

	AG_FileDlg* f = AG_FileDlgNew(fdwin,
			AG_FILEDLG_SAVE |  /* make sure the file is writable */
			AG_FILEDLG_EXPAND |
			AG_FILEDLG_CLOSEWIN /* close window when done */
		);

	/* Hook into the user choosing a file. file-chosen will give us
	 * both the path to the chosen file, and also the chosen file type. */
	AG_SetEvent(f, "file-chosen", ExportGraph,
				"%p(g),%p(statuslabel)", g, statuslabel);

	/* Set up valid output types. Note that the file type descriptions are
	 * string-matched to determine which AG_SurfaceExport function to call
	 * in ExportGraph -- they need to be changed in both places. */
	AG_FileDlgAddType(f, "PNG image", ".png", NULL, NULL);
	AG_FileDlgAddType(f, "JPEG image", ".jpg", NULL, NULL);
	AG_FileDlgAddType(f, "BPM image", ".bmp", NULL, NULL);

	/* display the file dialog window */
	AG_WindowShow(fdwin);

}

void graph_update(nocsim_state* state, AG_Driver* dri, AG_Box* box) {
	nocsim_node* cursor;
	nocsim_node* inner;
	unsigned int i;
	unsigned int j;
	AG_GraphVertex* vtx = NULL;
	AG_Graph* g;
	AG_Window* win;
	AG_Box* infobox;

	/* delete the existing network view so we have a clean state to work
	 * with */
	g = AG_GetPointer(dri, "graph_p");
	AG_ObjectDelete(g);
	g = AG_GraphNew(box, AG_GRAPH_EXPAND);
	AG_GraphSizeHint(g, 800, 600);
	AG_SetPointer(dri, "graph_p", g);

	/* this function will populate the info box */
	infobox = AG_GetPointer(dri, "infobox_p");
	AG_AddEvent(g, "graph-vertex-selected", HandleVertexSelection, NULL);

	/* force the newly created widget to draw */
	AG_WidgetHide(g);
	AG_WidgetShow(g);

	vec_foreach(state->nodes, cursor, i) {
		vtx = AG_GraphVertexNew(g, (void*) cursor);
		AG_GraphVertexLabelS(vtx, cursor->id);
		AG_GraphVertexPosition(vtx, cursor->col * 100, cursor->row * 100);
	}

	/* TODO: performance could be _much_ improved here */
	vec_foreach(state->nodes, cursor, i) {
		vec_foreach(state->nodes, inner, j) {
			if (i == j) {continue; }

			AG_GraphVertex* vtx1;
			AG_GraphVertex* vtx2;
			AG_GraphEdge* e;

			/* check if there is a link between these nodes */
			if (nocsim_link_by_nodes(state, cursor->id, inner->id) == NULL) {
				continue;
			}

			vtx1 = AG_GraphVertexFind(g, cursor);
			vtx2 = AG_GraphVertexFind(g, inner);
			if (vtx1 == NULL || vtx2 == NULL) { continue; }


			/* the link goes both ways */
			if (nocsim_link_by_nodes(state, inner->id, cursor->id) != NULL) {
				AG_GraphEdgeNew(g, vtx1, vtx2, NULL);
			} else {
				AG_DirectedGraphEdgeNew(g, vtx1, vtx2, NULL);
			}

		}
	}

}

void simulation_update(nocsim_state* state, AG_Driver* dri) {
	AG_Box* box = AG_GetPointer(dri, "siminfo_p");
	AG_Box* inner;
	AG_Object* parent = AG_ObjectParent(AGOBJECT(box));
	AG_ObjectDelete(box);
	box = AG_BoxNew(parent, AG_BOX_VERT, AG_BOX_EXPAND);
	AG_SetPointer(dri, "siminfo_p", box);


	inner = AG_BoxNew(box, AG_BOX_VERT, AG_BOX_FRAME | AG_BOX_HFILL);
	AG_BoxSetLabelS(inner, "general");

#define prval(label, fmt, ...) \
		AG_TextboxPrintf( \
			AG_TextboxNewS(inner, AG_TEXTBOX_READONLY | AG_TEXTBOX_HFILL, label), \
			fmt, __VA_ARGS__)

	prval("tick", "%lu", state->tick);
	prval("num_PE", "%u", state->num_PE);
	prval("num_router", "%u", state->num_router);
	prval("num_node", "%u", state->num_node);
	prval("flit_no", "%lu", state->flit_no);
	prval("max_row", "%u", state->max_row);
	prval("max_col", "%u", state->max_col);

	inner = AG_BoxNew(box, AG_BOX_VERT, AG_BOX_FRAME | AG_BOX_HFILL);
	AG_BoxSetLabelS(inner, "instruments");

	for (int i = 1 ; i < ENUMSIZE_INSTRUMENT ; i++) {
		if (state->instruments[i] == NULL) {
			prval(NOCSIM_INSTRUMENT_TO_STR(i), "%s", "(none)");
		} else {
			prval(NOCSIM_INSTRUMENT_TO_STR(i), "%s", state->instruments[i]);
		}
	}


	SV_WORKAROUND(box);
#undef prval

	AG_WidgetHide(box);
	AG_WidgetShow(box);

}


void EnterLine(AG_Event* event) {
	AG_Console* cons = AG_PTR(1);
	AG_Textbox *tb = AG_PTR(2);
	nocsim_state* state = AG_PTR(3);
	AG_Box* box = AG_PTR(4);
	char* s;

	AG_Color red;
	AG_ColorRGB_8(&red, 255, 16, 16);

	AG_Color green;
	AG_ColorRGB_8(&green, 16, 255, 16);

	s = AG_TextboxDupString(tb);


	if (Tcl_Eval(state->interp, s) == TCL_OK) {
		AG_ConsoleMsg(cons, "(OK   ) %% %s\t\t", s);
	} else {
		AG_ConsoleMsgColor(
			AG_ConsoleMsg(cons, "(ERROR) %% %s\t\t", s), &red);
		AG_ConsoleMsgColor(
		AG_ConsoleMsgS(cons, Tcl_GetStringResult(state->interp)), &red);
	}
	AG_TextboxSetString(tb, "");
	/* AG_Free(s); */

	graph_update(state, get_dri(), box);
	simulation_update(state, get_dri());

}

int main(int argc, char *argv[]) {
	AG_Window *win;
	AG_Graph* g;
	AG_Statusbar* statusbar;
	AG_Label* statuslabel;
	AG_Driver* dri;
	AG_MenuItem* temp;
	AG_Menu* menu;
	AG_Console* cons;
	AG_Button *btn;
	AG_Textbox *tb;
	nocsim_state* state;
	AG_Box* box;
	AG_Pane* outer_pane;
	AG_Pane* inner_pane;
	AG_Pane* infopane;

	state = nocsim_create_interp(NULL, argc, argv);

	/* Initialize LibAgar */
	if (AG_InitCore(NULL, 0) == -1 ||
			AG_InitGraphics(0) == -1)
		return (1);
	win = AG_WindowNew(0);
	AG_WindowSetCaptionS (win, "nocsim-gui");
	AG_WindowSetGeometry(win, -1, -1, 1200, 1000);

	/* setup the state handler and edge creation vertex variables */
	dri = (AG_Driver*) AG_ObjectRoot(win);

	menu = AG_MenuNew(win, 0);

	outer_pane = AG_PaneNewVert(win, AG_PANE_EXPAND);
	inner_pane = AG_PaneNewHoriz(outer_pane->div[0], AG_PANE_DIV1FILL | AG_PANE_EXPAND);

	/* instantiate the graph */
	g = AG_GraphNew(inner_pane->div[1], AG_GRAPH_EXPAND);
	AG_GraphSizeHint(g, 800, 600);
	AG_SetPointer(dri, "graph_p", g);

	/* Setup the status bar at the bottom of the window */
	AG_SeparatorNew(win, AG_SEPARATOR_HORIZ);
	statusbar = AG_StatusbarNew(win, AG_STATUSBAR_HFILL);
	statuslabel = AG_StatusbarAddLabel(statusbar, "");
	AG_LabelText(statuslabel, "ready");

	/* instantiate the "File" menu dropdown */
	AG_MenuItem* menu_file = AG_MenuNode(menu->root, "File", NULL);

	/* instantiate the contents of the File menu */
	{
		AG_MenuAction(menu_file, "Export", NULL, ExportGraphDialog,
				"%p(g),%p(statuslabel)", g, statuslabel);

		AG_MenuSeparator(menu_file);

		AG_MenuAction(menu_file, "Quit", NULL, QuitApplication, NULL);

#ifdef ENABLE_DEBUGGER
		AG_MenuSeparator(menu_file);
		AG_MenuAction(menu_file, "Launch Debugger", NULL, LaunchDebugger, NULL);
#endif
	}

	/* setup the console */
	cons = AG_ConsoleNew(outer_pane->div[1], AG_CONSOLE_EXPAND);
	AG_SetStyle(cons, "font-family", "Courier");

	/* setup text entry box */
	box = AG_BoxNewHoriz(outer_pane->div[1], AG_BOX_HFILL);
	{
		tb = AG_TextboxNew(box,
		    AG_TEXTBOX_EXCL | AG_TEXTBOX_HFILL | AG_TEXTBOX_VFILL,
		    "Input: ");
		AG_SetEvent(tb, "textbox-return", EnterLine, "%p,%p,%p,%p", cons, tb, state, inner_pane->div[1]);
		AG_WidgetFocus(tb);

		btn = AG_ButtonNewFn(box, 0, "Enter", EnterLine, "%p,%p,%p,%p", cons, tb, state,inner_pane->div[1]);
		AG_WidgetSetFocusable(btn, 0);
	}

	/* display the window */
	AG_WindowShow(win);

	/* split up node info view and simulation info view */
	infopane = AG_PaneNewVert(inner_pane->div[0], AG_PANE_EXPAND);
	AG_PaneMoveDividerPct(infopane, 50);

	/* info view area */
	box = AG_BoxNew(
			AG_ScrollviewNew(infopane->div[0], AG_SCROLLVIEW_EXPAND),
			AG_BOX_VERT, AG_BOX_EXPAND);
	AG_SetPointer(dri, "infobox_p", box);

	/* this function will populate the info box */
	AG_AddEvent(g, "graph-vertex-selected", HandleVertexSelection, NULL);

	/* simulation info view */
	box = AG_BoxNew(
			AG_ScrollviewNew(infopane->div[1], AG_SCROLLVIEW_EXPAND),
			AG_BOX_VERT, AG_BOX_EXPAND);
	AG_SetPointer(dri, "siminfo_p", box);
	simulation_update(state, dri);


	AG_EventLoop();
	return (0);
}
#undef SV_WORKAROUND
