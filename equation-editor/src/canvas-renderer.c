/* -*- mode: c; style: linux -*- */

/* canvas-renderer.c
 * Copyright (C) 2000 Helix Code, Inc.
 *
 * Written by Bradford Hovinen <hovinen@helixcode.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "canvas-renderer.h"

enum {
	ARG_0,
	ARG_CANVAS
};

struct _CanvasRendererPrivate 
{
	GtkWidget *canvas;
	GdkFont *font;
};

static RendererClass *parent_class;

static void canvas_renderer_init            (CanvasRenderer *canvas_renderer);
static void canvas_renderer_class_init      (CanvasRendererClass *class);

static void canvas_renderer_set_arg         (GtkObject *object, 
					     GtkArg *arg, 
					     guint arg_id);
static void canvas_renderer_get_arg         (GtkObject *object, 
					     GtkArg *arg, 
					     guint arg_id);

static void canvas_renderer_finalize        (GtkObject *object);





static void canvas_renderer_render_line     (Renderer *renderer,
					     gdouble x1, gdouble y1, 
					     gdouble x2, gdouble y2,
					     gdouble thickness);
static void canvas_renderer_render_glyph    (Renderer *renderer,
					     gint code, gdouble x, gdouble y,
					     gdouble scale);
static void canvas_renderer_render_number   (Renderer *renderer,
					     gdouble value,
					     gdouble x, gdouble y,
					     gdouble scale, gdouble pres);
static void canvas_renderer_render_string   (Renderer *renderer,
					     const gchar *string, 
					     gdouble x, gdouble y,
					     gdouble scale);

static void canvas_renderer_get_glyph_geom  (Renderer *renderer, gint code,
					     gdouble *width, gdouble *height,
					     gdouble *ascent,
					     gdouble *descent);
static void canvas_renderer_get_number_geom (Renderer *renderer, gdouble value,
					     gdouble *width, gdouble *height,
					     gdouble *ascent, 
					     gdouble *descent);
static void canvas_renderer_get_string_geom (Renderer *renderer, gchar *string,
					     gdouble *width, gdouble *height,
					     gdouble *ascent,
					     gdouble *descent);

guint
canvas_renderer_get_type (void)
{
	static guint canvas_renderer_type = 0;

	if (!canvas_renderer_type) {
		GtkTypeInfo canvas_renderer_info = {
			"CanvasRenderer",
			sizeof (CanvasRenderer),
			sizeof (CanvasRendererClass),
			(GtkClassInitFunc) canvas_renderer_class_init,
			(GtkObjectInitFunc) canvas_renderer_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		canvas_renderer_type = 
			gtk_type_unique (renderer_get_type (), 
					 &canvas_renderer_info);
	}

	return canvas_renderer_type;
}

static void
canvas_renderer_init (CanvasRenderer *canvas_renderer)
{
	canvas_renderer->p = g_new0 (CanvasRendererPrivate, 1);
	canvas_renderer->p->font =
		gdk_font_load
		("-adobe-helvetica-medium-r-normal--24-*-*-*-*-*-iso8859-1");
}

static void
canvas_renderer_class_init (CanvasRendererClass *class) 
{
	GtkObjectClass *object_class;
	RendererClass *renderer_class;

	gtk_object_add_arg_type ("CanvasRenderer::canvas",
				 GTK_TYPE_POINTER,
				 GTK_ARG_READWRITE,
				 ARG_CANVAS);

	object_class = GTK_OBJECT_CLASS (class);
	object_class->finalize = canvas_renderer_finalize;
	object_class->set_arg = canvas_renderer_set_arg;
	object_class->get_arg = canvas_renderer_get_arg;

	renderer_class = RENDERER_CLASS (class);
	renderer_class->render_line = canvas_renderer_render_line;
	renderer_class->render_glyph = canvas_renderer_render_glyph;
	renderer_class->render_number = canvas_renderer_render_number;
	renderer_class->render_string = canvas_renderer_render_string;

	renderer_class->get_glyph_geom = canvas_renderer_get_glyph_geom;
	renderer_class->get_number_geom = canvas_renderer_get_number_geom;
	renderer_class->get_string_geom = canvas_renderer_get_string_geom;

	parent_class = RENDERER_CLASS
		(gtk_type_class (renderer_get_type ()));
}

static void
canvas_renderer_set_arg (GtkObject *object, GtkArg *arg, guint arg_id) 
{
	CanvasRenderer *canvas_renderer;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_CANVAS_RENDERER (object));

	canvas_renderer = CANVAS_RENDERER (object);

	switch (arg_id) {
	case ARG_CANVAS:
		g_return_if_fail (GTK_VALUE_POINTER (*arg) == NULL ||
				  GTK_IS_WIDGET (GTK_VALUE_POINTER (*arg)));

		if (canvas_renderer->p->canvas != NULL)
			gtk_object_unref
				(GTK_OBJECT (canvas_renderer->p->canvas));

		canvas_renderer->p->canvas = GTK_VALUE_POINTER (*arg);

		if (canvas_renderer->p->canvas != NULL)
			gtk_object_ref
				(GTK_OBJECT (canvas_renderer->p->canvas));
		break;

	default:
		g_warning ("Bad argument set");
		break;
	}
}

static void
canvas_renderer_get_arg (GtkObject *object, GtkArg *arg, guint arg_id) 
{
	CanvasRenderer *canvas_renderer;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_CANVAS_RENDERER (object));

	canvas_renderer = CANVAS_RENDERER (object);

	switch (arg_id) {
	case ARG_CANVAS:
		GTK_VALUE_POINTER (*arg) = canvas_renderer->p->canvas;
		break;

	default:
		g_warning ("Bad argument get");
		break;
	}
}

static void
canvas_renderer_finalize (GtkObject *object) 
{
	CanvasRenderer *canvas_renderer;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_CANVAS_RENDERER (object));

	canvas_renderer = CANVAS_RENDERER (object);

	g_free (canvas_renderer->p);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkObject *
canvas_renderer_new (GtkWidget *canvas) 
{
	return gtk_object_new (canvas_renderer_get_type (),
			       "canvas", canvas,
			       NULL);
}

static void
canvas_renderer_render_line (Renderer *renderer,
			     gdouble x1, gdouble y1, 
			     gdouble x2, gdouble y2,
			     gdouble thickness)
{
	CanvasRenderer *canvas_renderer;

	canvas_renderer = CANVAS_RENDERER (renderer);

	if (!GTK_WIDGET_REALIZED (canvas_renderer->p->canvas)) return;

	gdk_draw_line (canvas_renderer->p->canvas->window,
		       canvas_renderer->p->canvas->style->black_gc,
		       x1, y1, x2, y2);
}

static void
canvas_renderer_render_glyph (Renderer *renderer,
			      gint code, gdouble x, gdouble y,
			      gdouble scale)
{
	CanvasRenderer *canvas_renderer;
	char buffer[2];

	canvas_renderer = CANVAS_RENDERER (renderer);

	if (!GTK_WIDGET_REALIZED (canvas_renderer->p->canvas)) return;

	buffer[0] = code; buffer[1] = '\0';
	gdk_draw_text (canvas_renderer->p->canvas->window,
		       canvas_renderer->p->font,
		       canvas_renderer->p->canvas->style->black_gc,
		       (gint) x + 50, (gint) y + 50, buffer, 1);
}

static void
canvas_renderer_render_number (Renderer *renderer,
			       gdouble value, gdouble x, gdouble y,
			       gdouble scale, gdouble pres)
{
	CanvasRenderer *canvas_renderer;
	gchar buffer[1024], fmtstring[1024];

	canvas_renderer = CANVAS_RENDERER (renderer);

	if (!GTK_WIDGET_REALIZED (canvas_renderer->p->canvas)) return;

	g_snprintf (fmtstring, 1024, "%%%df", (int) pres);
	g_snprintf (buffer, 1024, fmtstring, value);
	gdk_draw_text (canvas_renderer->p->canvas->window,
		       canvas_renderer->p->font,
		       canvas_renderer->p->canvas->style->black_gc,
		       (gint) x + 50, (gint) y + 50, buffer, 1);
}

static void
canvas_renderer_render_string (Renderer *renderer,
			       const gchar *string, gdouble x, gdouble y,
			       gdouble scale)
{
	CanvasRenderer *canvas_renderer;

	canvas_renderer = CANVAS_RENDERER (renderer);

	if (!GTK_WIDGET_REALIZED (canvas_renderer->p->canvas)) return;

	gdk_draw_text (canvas_renderer->p->canvas->window,
		       canvas_renderer->p->font,
		       canvas_renderer->p->canvas->style->black_gc,
		       x + 50, y + 50, string, 1);
}

static void
canvas_renderer_get_glyph_geom (Renderer *renderer, gint code,
				gdouble *width, gdouble *height,
				gdouble *ascent, gdouble *descent)
{
	CanvasRenderer *canvas_renderer;

	canvas_renderer = CANVAS_RENDERER (renderer);

	if (width != NULL)
		*width = gdk_char_width (canvas_renderer->p->font, code);
	if (height != NULL)
		*height = gdk_char_height (canvas_renderer->p->font, code);
}

static void
canvas_renderer_get_number_geom (Renderer *renderer, gdouble value,
				 gdouble *width, gdouble *height,
				 gdouble *ascent, gdouble *descent)
{
	CanvasRenderer *canvas_renderer;
	gchar buffer[1024];

	canvas_renderer = CANVAS_RENDERER (renderer);

	g_snprintf (buffer, 1024, "%d", (gint) value);

	if (width != NULL)
		*width = gdk_string_width (canvas_renderer->p->font, buffer);
	if (height != NULL)
		*height = gdk_string_height (canvas_renderer->p->font, buffer);
}

static void
canvas_renderer_get_string_geom (Renderer *renderer, gchar *string,
				 gdouble *width, gdouble *height,
				 gdouble *ascent, gdouble *descent)
{
	CanvasRenderer *canvas_renderer;

	canvas_renderer = CANVAS_RENDERER (renderer);

	if (width != NULL)
		*width = gdk_string_width (canvas_renderer->p->font, string);
	if (height != NULL)
		*height = gdk_string_height (canvas_renderer->p->font, string);
}
