#include "text_widget.h"

NV_TextWidget* NV_TextWidgetNew(void* parent, char* label, nocviz_graph* g, char* id, char* key) {
	NV_TextWidget* tw;

	tw = noctools_malloc(sizeof(NV_TextWidget));


	/* dupe strings, since this widget may have a different life cycle
	 * than the corresponding node object -- this is the same reason
	 * we identify our node by ID, rather than by it's address */
	tw->label_text = strdup(label);
	tw->key = strdup(key);
	if (id != NULL) {
		tw->id = strdup(id);
	} else {
		tw->id = NULL;
	}
	tw->id2 = NULL;

	tw->g = g;

	/* note that tw->tb gets set up in Init */
	AG_ObjectInit(tw, &NV_TextWidgetClass);

	/* connect to the VFS */
	AG_ObjectAttach(parent, tw);

	return tw;
}

static void SizeRequest(void *p, AG_SizeReq* r) {
	NV_TextWidget* this = p;

	/* only care about the child widget size */
	AG_WidgetSizeReq(this->tb, r);
}

static int SizeAllocate(void* p, const AG_SizeAlloc* a) {
	NV_TextWidget* this = p;
	AG_SizeAlloc aChld;
	AG_SizeReq rChld;

	/* see how big the child wants to be */
	AG_WidgetSizeReq(this->tb, &rChld);

	/* copy it over to the allocation */
	aChld.w = rChld.w;
	aChld.h = rChld.h;
	aChld.x = 0;
	aChld.y = 0;

	/* if the child wanted to be bigger than the space we have available,
	 * set it to the max space we can get */
	if (aChld.w > a->w) {aChld.w = a->w;}
	if (aChld.h > a->h) {aChld.h = a->h;}

	/* tell the child how much space we allocated for it */
	AG_WidgetSizeAlloc(this->tb, &aChld);

	return 0;
}

static void Draw(void* p) {
	NV_TextWidget* this = p;
	nocviz_node* n;
	nocviz_link* l;
	nocviz_ds* ds;

	if (this->id2 == NULL && this->id == NULL) {
		/* global view */
		ds = this->g->ds;
	} else if (this->id2 == NULL) {
		/* this refers to a node */
		n = nocviz_graph_get_node(this->g, (char*) this->id);
		ds = n->ds;
	} else {
		/* this refers to a link */
		l = nocviz_graph_get_link(this->g, (char*) this->id, (char*) this->id2);
		ds = l->ds;
	}

	char* msg;

	/* allocate an appropriate message based on weather or not the node was
	 * found */
	if (ds == NULL) {
		if (this->id2 == NULL) {
			if (asprintf(&msg, "ERROR: no such node '%s'   ", this->id) < 0) {
				return;
			}
		} else {
			if (asprintf(&msg, "ERROR: no such link '%s' -> '%s'   ", this->id, this->id2) < 0) {
				return;
			}
		}

	} else {
		if (asprintf(&msg, "%s   ", nocviz_ds_format(ds, (char*) this->key)) < 0) {
			return;
		}
	}

	/* update textbox contents */
	AG_TextboxPrintf(this->tb, "%s", msg);

	/* update textbox requested size */
	AG_EditableSizeHint(this->tb->ed, msg);

	/* redraw the textbox */
	AG_WidgetDraw(this->tb);
	AG_WidgetHide(this->tb);
	AG_WidgetShow(this->tb);

	free(msg);

}

static void Init(void * obj) {
	NV_TextWidget *this = obj;

	/* instantiate child textbox */
	this->tb = AG_TextboxNewS(AGOBJECT(this),
			AG_TEXTBOX_READONLY | AG_TEXTBOX_HFILL, this->label_text);
	this->tb->ed->flags |= AG_EDITABLE_HFILL;

	AG_SetStyle(this->tb, "color", "rgb(196,196,196)");
	AG_SetStyle(this->tb->ed, "color", "rgb(196,196,196)");
	AG_SetStyle(this->tb, "text-color", "rgba(36,36,36,0)");
	AG_SetStyle(this->tb->ed, "text-color", "rgb(36,36,36,0)");

	AG_SetStyle(this->tb, "color#hover", "rgb(196,196,196)");
	AG_SetStyle(this->tb->ed, "color#hover", "rgb(196,196,196)");
	AG_SetStyle(this->tb, "text-color#hover", "rgba(36,36,36,0)");
	AG_SetStyle(this->tb->ed, "text-color#hover", "rgb(36,36,36,0)");

	AG_SetStyle(this->tb, "color#focused", "rgb(196,196,196)");
	AG_SetStyle(this->tb->ed, "color#focused", "rgb(196,196,196)");
	AG_SetStyle(this->tb, "text-color#focused", "rgba(36,36,36,0)");
	AG_SetStyle(this->tb->ed, "text-color#focused", "rgb(36,36,36,0)");

	AGWIDGET(this)->flags |= AG_WIDGET_FOCUSABLE;

	/* redraw at ~60FPS */
	AG_RedrawOnTick(this, 1000/60);

}

static void Free(void* obj) {
	NV_TextWidget *this = obj;

	/* release duped strings */
	free(this->label_text);
	free(this->id);
	free(this->key);

	if (this->id2 != NULL) {
		free(this->id2);
	}

	/* this->tb will be deallocated by it's own Free */
}

AG_WidgetClass NV_TextWidgetClass = {
	{
		"AG_Widget:NV_TextWidget",	/* Name of class */
		sizeof(NV_TextWidget),	/* Size of structure */
		{ 0,0 },		/* Version for load/save */
		Init,			/* Initialize dataset */
		Free,			/* Free dataset */
		NULL,			/* Destroy widget */
		NULL,			/* Load widget (for GUI builder) */
		NULL,			/* Save widget (for GUI builder) */
		NULL,			/* Edit (for GUI builder) */
		"NV_TextWidget",
		&agWidgetClass._inherit,
		{{0}, {0}, {0}}
	},
	Draw,				/* Render widget */
	SizeRequest,			/* Default size requisition */
	SizeAllocate			/* Size allocation callback */
};
