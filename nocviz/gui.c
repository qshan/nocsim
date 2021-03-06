#include "gui.h"

void gui_init(nocviz_gui_handle* h, Tcl_Interp* interp, nocviz_graph* graph) {
	nocviz_gui_params* p = noctools_malloc(sizeof(nocviz_gui_params));
	AG_Thread th;

	p->interp = interp;
	p->graph = graph;

	AG_ThreadCreate(&th, gui_main, p);

	h->thread = th;
	h->params = p;
}

void QuitApplication(AG_Event *event) {
	UNUSED(event);
	AG_DestroyGraphics();
	AG_Destroy();
	AG_Quit();
	AG_ThreadExit(NULL);
}

/* If the debugger is enabled, the "Launch Debugger" menu item will use this
 * to display the LibAgar debugging window */
#ifdef ENABLE_DEBUGGER
void LaunchDebugger(AG_Event *event) {
	AG_Driver* dri;
	AG_Window* db_window;
	AG_Window* win;

	/* Fetch the top-level driver object, which is also the parent of the
	 * main window. We will attach the debugger window to this object. */
	dri = (AG_Driver*) get_dri();

	win = AG_GetPointer(dri, "main_window");

	db_window = AG_GuiDebugger(win);
	AG_WindowShow(db_window);
}
#endif

void ToggleEdgeLabels(AG_Event* event) {
	AG_Driver* dri = get_dri();

	int* show_edge_labels;
	show_edge_labels = AG_GetPointer(dri, "show_edge_labels");

	if (*show_edge_labels == 0) {
		*show_edge_labels = 1;
	} else {
		*show_edge_labels = 0;
	}

}

void ToggleNodeLabels(AG_Event* event) {
	AG_Driver* dri = get_dri();

	int* show_node_labels;
	show_node_labels = AG_GetPointer(dri, "show_node_labels");

	if (*show_node_labels == 0) {
		*show_node_labels = 1;
	} else {
		*show_node_labels = 0;
	}
}

/* handler where we no longer have a vertex or edge selected */
void handle_deselection(nocviz_graph* g_data, AG_Driver* dri) {
	AG_Box* infobox;
	AG_Box* sectionbox;
	strvec* section;
	const char* sectionname;
	AG_Object* parent;
	char* key;
	unsigned int i;

	/* vtx->userPtr should be safe now */
	AG_SetPointer(dri, "selected_node", NULL);
	/* invalidate */
	AG_SetPointer(dri, "selected_link", NULL);

	/* delete the existing info box and create a new one */
	infobox = AG_GetPointer(dri, "infobox_p");
	parent = AG_ObjectParent(AGOBJECT(infobox));
	AG_ObjectDelete(infobox);
	infobox = AG_BoxNew(parent, AG_BOX_VERT, AG_BOX_EXPAND);
	AG_SetPointer(dri, "infobox_p", infobox);

#ifdef EBUG
	sectionbox = AG_BoxNew(infobox, AG_BOX_VERT, AG_BOX_HFILL | AG_BOX_FRAME);
	AG_BoxSetLabelS(sectionbox, "DEBUG DEBUG DEBUG");
	AG_LabelNew(sectionbox, AG_LABEL_HFILL, "parent=%p", (void*) parent);
	AG_LabelNew(sectionbox, AG_LABEL_HFILL, "infobox_p=%p", (void*) infobox);
	AG_LabelNew(sectionbox, AG_LABEL_HFILL, "graph=%p", (void*) g_data);
#endif

	/* generate the info panel contents */
	nocviz_ds_foreach_section(g_data->ds, sectionname, section,
		sectionbox = AG_BoxNew(infobox, AG_BOX_VERT, AG_BOX_HFILL | AG_BOX_FRAME);
		AG_BoxSetLabel(sectionbox, "%s", sectionname);
		vec_foreach(section, key, i) {
			/* NV_TextWidget comes from text_widget.{c,h}, and
			 * will automatically keep polling the given node ID
			 * on it's own */
			NV_TextWidgetNew(sectionbox, key, g_data, NULL, key);
		}
	);

	/* workaround to force the widget to redraw immediately */
	AG_WidgetHide(infobox);
	AG_WidgetShow(infobox);
}

void clear_selection_button_handler(AG_Event* event) {
	AG_Driver* dri = get_dri();
	nocviz_graph* g_data = AG_PTR_NAMED("g_data");

	handle_deselection(g_data, dri);
}


void* gui_main(void* arg) {
	nocviz_gui_params* p = arg;

	AG_Window *win;
	NV_GraphWidget* gw;
	AG_Driver* dri;
	AG_Menu* menu;
	AG_Box* box;
	AG_Pane* inner_pane;
	AG_Toolbar* tb;
	/* AG_StyleSheet* ss; */

	/* setup custom widgets */
	int show_node_labels = 1;
	int show_edge_labels = 0;

	/* Initialize LibAgar */
	if (AG_InitCore(NULL, 0) == -1 ||
			AG_InitGraphics(NULL) == -1)
		return NULL;

	AG_LoadStyleSheet(NULL, "style.css");

	win = AG_WindowNew(0);
	AG_WindowSetCaptionS (win, "nocviz-gui");
	AG_WindowSetGeometry(win, -1, -1, 1100, 900);

	/* setup the state handler and edge creation vertex variables */
	dri = (AG_Driver*) AG_ObjectRoot(win);

	AG_RegisterClass(&NV_TextWidgetClass);
	AG_RegisterClass(&NV_GraphWidgetClass);

	AG_SetPointer(dri, "main_window", win);

	menu = AG_MenuNew(win, 0);

	inner_pane = AG_PaneNewHoriz(win, AG_PANE_DIV1FILL | AG_PANE_EXPAND);

	/* setup the toolbar for global ops */
	tb = AG_ToolbarNew(inner_pane->div[1], AG_TOOLBAR_HORIZ, 1, AG_TOOLBAR_HFILL);
	AG_ToolbarButton(tb, "clear selection", 1,
			clear_selection_button_handler, "%p(g_data)", p->graph);

	/* instantiate the graph */
	gw = NV_GraphWidgetNew(inner_pane->div[1], p->graph);
	NV_GraphSizeHint(gw, NOCVIZ_GUI_GRAPH_DEFAULT_WIDTH,
			NOCVIZ_GUI_GRAPH_DEFAULT_HEIGHT);
	AG_RedrawOnTick(gw, 50);
	AG_AddEvent(gw, "graph-vertex-selected",
			handle_vertex_selection, "%p(nocviz_graph)", gw->g);
	AG_AddEvent(gw, "graph-edge-selected",
			handle_link_selection, "%p(nocviz_graph)", gw->g);

	AG_SetPointer(dri, "graph_p", gw);

	/* global flags for edge/node labels */
	AG_SetPointer(dri, "show_node_labels", &show_node_labels);
	AG_SetPointer(dri, "show_edge_labels", &show_edge_labels);

	/* global to track currently selected node */
	AG_SetPointer(dri, "selected_node", NULL);
	AG_SetPointer(dri, "selected_link", NULL);

	/* instantiate the "File" menu dropdown */
	AG_MenuItem* menu_file = AG_MenuNode(menu->root, "File", NULL);

	/* instantiate the contents of the File menu */
	{
		AG_MenuAction(menu_file, "Quit", NULL, QuitApplication, NULL);

#ifdef ENABLE_DEBUGGER
		AG_MenuSeparator(menu_file);
		AG_MenuAction(menu_file, "Launch Debugger", NULL, LaunchDebugger, NULL);
#endif
	}

	/* instantiate the "View" menu dropdown */
	AG_MenuItem* menu_view = AG_MenuNode(menu->root, "View", NULL);

	{
		AG_MenuAction(menu_view, "Toggle Edge Labels", NULL, ToggleEdgeLabels, "%p(g_data)", p->graph);
		AG_MenuAction(menu_view, "Toggle Node Labels", NULL, ToggleNodeLabels, "%p(g_data)", p->graph);
	}

	/* info view area */
	box = AG_BoxNew(
			AG_ScrollviewNew(inner_pane->div[0], AG_SCROLLVIEW_BY_MOUSE | AG_SCROLLVIEW_EXPAND),
			AG_BOX_VERT, AG_BOX_EXPAND);
	AG_SetPointer(dri, "infobox_p", box);

	/* show the deselected view by default */
	handle_deselection(p->graph, dri);

	AG_WindowShow(win);


	AG_EventLoop();

	return NULL;
}
