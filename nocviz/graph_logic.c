#include "gui.h"

/* update the graph widget to be consistent with the underlying nocviz graph
 * state */
unsigned int graph_update_handler(AG_Timer* to, AG_Event* event) {
	UNUSED(to);
	AG_Driver* dri = get_dri();
	AG_Window* win = AG_GetPointer(dri, "main_window");
	AG_Timer* new_to;
	NV_GraphWidget* g_wid = AG_GetPointer(dri, "graph_p");

	nocviz_gui_params* p = AG_PTR_NAMED("gui_param");
	nocviz_graph* g_data = p->graph;

	/* update only if the graph is marked dirty */
	if (nocviz_graph_is_dirty(g_data)) {
		/* graph_update(dri, g_data); */
		AG_WidgetHide(g_wid);
		AG_WidgetShow(g_wid);
	}

	if (nocviz_graph_color_is_dirty(g_data)) {
		/* graph_color_update(dri, g_data); */
		AG_WidgetHide(g_wid);
		AG_WidgetShow(g_wid);
	}

	/* instantiate a new timer and start it running so it will call us
	 * later. */
	new_to = noctools_malloc(sizeof(AG_Timer));
	AG_InitTimer(new_to, "graph_update_event", AG_TIMER_AUTO_FREE);
	AG_AddTimer(win, new_to, NOCVIZ_GUI_GRAPH_UPDATE_INTERVAL,
			graph_update_handler, "%p(gui_param)", p);

	return 0;
}

/* handle the user selecting a vertex, remember that the userPtr field of
 * the vertices is a pointer to the corresponding nocviz_node.
 *
 * XXX: it is possible that if a node is deleted while this function is
 * executing, it may attempt to read from unallocated memory.
 *
 * */
void handle_vertex_selection(AG_Event* event) {
	/* AddEvent appends automatic arguments to the end */
	nocviz_node* vtx = AG_PTR(2);
	nocviz_graph* g_data = AG_PTR_NAMED("nocviz_graph");
	AG_Driver* dri = get_dri();
	AG_Box* infobox;
	AG_Box* sectionbox;
	strvec* section;
	const char* sectionname;
	AG_Object* parent;
	char* key;
	unsigned int i;

	/* if a node was deleted from the underlying data structure before
	 * graph_update was called via it's handler, then vtx->userPtr will
	 * point to un-allocated memory. This guarantees that cannot happen.
	 */
	/* if (nocviz_graph_is_dirty(g_data)) { */
		/* graph_update(dri, g_data); */
	/* } */

	/* vtx->userPtr should be safe now */
	AG_SetPointer(dri, "selected_node", vtx);
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
	AG_LabelNew(sectionbox, AG_LABEL_HFILL, "selected_node=%p", (void*) vtx);
	AG_LabelNew(sectionbox, AG_LABEL_HFILL, "node title=%s", vtx->title);
#endif

	/* generate the info panel contents */
	nocviz_ds_foreach_section(vtx->ds, sectionname, section,
		sectionbox = AG_BoxNew(infobox, AG_BOX_VERT, AG_BOX_HFILL | AG_BOX_FRAME);
		AG_BoxSetLabel(sectionbox, "%s", sectionname);
		vec_foreach(section, key, i) {
			/* NV_TextWidget comes from text_widget.{c,h}, and
			 * will automatically keep polling the given node ID
			 * on it's own */
			NV_TextWidgetNew(sectionbox, key, g_data, vtx->id, key);
		}
	);

	/* workaround to force the widget to redraw immediately */
	AG_WidgetHide(infobox);
	AG_WidgetShow(infobox);

}

void handle_link_selection(AG_Event* event) {
	/* AddEvent appends automatic arguments to the end */
	AG_GraphEdge* edge = AG_PTR(2);
	nocviz_graph* g_data = AG_PTR_NAMED("nocviz_graph");
	AG_Driver* dri = get_dri();
	AG_Box* infobox;
	AG_Box* sectionbox;
	strvec* section;
	const char* sectionname;
	AG_Object* parent;
	nocviz_link* link = edge->userPtr;
	char* key;
	unsigned int i;

	/* if a node was deleted from the underlying data structure before
	 * graph_update was called via it's handler, then vtx->userPtr will
	 * point to un-allocated memory. This guarantees that cannot happen.
	 */
	if (nocviz_graph_is_dirty(g_data)) {
		graph_update(dri, g_data);
	}

	/* link->userPtr should be safe now */
	AG_SetPointer(dri, "selected_link", edge->userPtr);

	/* invalidate */
	AG_SetPointer(dri, "selected_node", NULL);

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
	AG_LabelNew(sectionbox, AG_LABEL_HFILL, "selected_link=%p", (void*) link);
	AG_LabelNew(sectionbox, AG_LABEL_HFILL, "link title=%s", link->title);
#endif

	/* generate the info panel contents */
	nocviz_ds_foreach_section(link->ds, sectionname, section,
		sectionbox = AG_BoxNew(infobox, AG_BOX_VERT, AG_BOX_HFILL | AG_BOX_FRAME);
		AG_BoxSetLabel(sectionbox, "%s", sectionname);
		vec_foreach(section, key, i) {
			/* NV_TextWidget comes from text_widget.{c,h}, and
			 * will automatically keep polling the given node ID
			 * on it's own */
			NV_TextWidgetNew(sectionbox,
					key,
					g_data,
					link->from->id,
					key)->id2 = strdup(link->to->id);
			/* ->id2 is how NV_TextWidget knows we are referring
			 * to a link rather than a node */

		}
	);

	/* workaround to force the widget to redraw immediately */
	AG_WidgetHide(infobox);
	AG_WidgetShow(infobox);

}


/* actually do the work of updating the graph widget to reflect the contents of
 * the graph data structure
 *
 * XXX: handling of link/node titles may not be thread safe */
void graph_update(AG_Driver* dri, nocviz_graph* g_data) {
	AG_Graph* g_wid = AG_GetPointer(dri, "graph_p");
	AG_Object* g_parent;
	AG_GraphVertex* vtx;
	nocviz_node* n;
	nocviz_link* l;
	AG_GraphEdge* edge;

	/* if a node with this userPtr still exists after we re-generate the
	 * graph widget, we will re-select it */
	nocviz_node* selected_node = AG_GetPointer(dri, "selected_node");
	nocviz_link* selected_link = AG_GetPointer(dri, "selected_link");
	int xOffs;
	int yOffs;

	/* configure if node/edge labels should be shown
	 *
	 * XXX: currently, there are no edge labels */
	int* show_edge_labels;
	int* show_node_labels;
	show_edge_labels = AG_GetPointer(dri, "show_edge_labels");
	show_node_labels = AG_GetPointer(dri, "show_node_labels");

	dbprintf("dirty graph! performing update... \n");

	/* mark as clean */
	nocviz_graph_set_dirty(g_data, false);

	/* save the current offset so it can be re-applied later */
	xOffs = g_wid->xOffs;
	yOffs = g_wid->yOffs;

	/* save the widget's parent so we know where to put the new one */
	g_parent = AG_ObjectParent(AGOBJECT(g_wid));

	AG_ObjectDelete(g_wid);

	/* setup the new widget */
	g_wid = AG_GraphNew(g_parent, AG_GRAPH_EXPAND);
	AG_SetPointer(dri, "graph_p", g_wid);
	AG_GraphSizeHint(g_wid, NOCVIZ_GUI_GRAPH_DEFAULT_WIDTH,
			NOCVIZ_GUI_GRAPH_DEFAULT_HEIGHT);

	/* update it's x/y offset to the saved copy */
	g_wid->xOffs = xOffs;
	g_wid->yOffs = yOffs;

	/* create all nodes */
	nocviz_graph_foreach_node(g_data, n,
		vtx = AG_GraphVertexNew(g_wid, n);
		if (*show_node_labels == 1) {
			AG_GraphVertexLabel(vtx, "%s", n->title);
		}
		AG_GraphVertexPosition(vtx, n->col* 50, n->row* 50);
	);

	/* create all links */
	nocviz_graph_foreach_link(g_data, l,
		if ((l->from == NULL) || (l->to == NULL)) {
			err(1, "link %s not connected at both endpoints", l->title);
		}

		if ((AG_GraphVertexFind(g_wid, l->from) == NULL) ||
			(AG_GraphVertexFind(g_wid, l->to) == NULL)) {
			/* the corresponding graph nodes have not been created
			 * yet */
			continue;
		}
		if (l->type == NOCVIZ_LINK_UNDIRECTED) {
			edge = AG_GraphEdgeNew(g_wid,
					AG_GraphVertexFind(g_wid, l->from),
					AG_GraphVertexFind(g_wid, l->to),
					l);
		} else {
			edge = AG_DirectedGraphEdgeNew(g_wid,
					AG_GraphVertexFind(g_wid, l->from),
					AG_GraphVertexFind(g_wid, l->to),
					l);
		}
		if (*show_edge_labels == 1) {
			AG_GraphEdgeLabel(edge, "%s", l->title);
		}
	);

	/* re-select the previously selected vertex, if it still exists */
	if (selected_node != NULL) {
		vtx = AG_GraphVertexFind(g_wid, selected_node);
		if (vtx != NULL) {
			vtx->flags |= AG_GRAPH_SELECTED;
		} else {
			AG_SetPointer(dri, "selected_node", NULL);
		}
	}

	if (selected_link != NULL) {
		edge = AG_GraphEdgeFind(g_wid, selected_link);
		if (edge != NULL) {
			edge->flags |= AG_GRAPH_SELECTED;
		} else {
			AG_SetPointer(dri, "selected_link", NULL);
		}
	}

	/* re-register the event handler */
	AG_AddEvent(g_wid, "graph-vertex-selected",
			handle_vertex_selection, "%p(nocviz_graph)", g_data);
	AG_AddEvent(g_wid, "graph-edge-selected",
			handle_link_selection, "%p(nocviz_graph)", g_data);

	/* workaround to force the widget to redraw immediately */
	AG_WidgetHide(g_wid);
	AG_WidgetShow(g_wid);

	dbprintf("graph update completed.\n");

}

void graph_color_update(AG_Driver* dri, nocviz_graph* g_data) {
	AG_Graph* g_wid = AG_GetPointer(dri, "graph_p");
	AG_GraphVertex* vtx;
	nocviz_node* n;
	nocviz_link* l;
	AG_GraphEdge* edge;

	nocviz_graph_color_set_dirty(g_data, false);

	/* color all nodes */
	nocviz_graph_foreach_node(g_data, n,
		vtx = AG_GraphVertexFind(g_wid, n);
		if (vtx != NULL) {
			AG_ObjectLock(vtx->graph);
			vtx->bgColor = n->c;
			AG_ObjectUnlock(vtx->graph);
			AG_Redraw(g_wid);
		}
	);

	/* color all links */
	nocviz_graph_foreach_link(g_data, l,
		edge = AG_GraphEdgeFind(g_wid, l);
		if (edge != NULL) {
			AG_ObjectLock(edge->graph);
			edge->edgeColor = l->c;
			AG_ObjectUnlock(edge->graph);
			AG_Redraw(g_wid);
		}
	);

	AG_WidgetHide(g_wid);
	AG_WidgetShow(g_wid);

}
