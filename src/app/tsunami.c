/***********************************
 * DreamShell ##version##          *
 * tsunami.c                       *
 * DreamShell Tsunami App loader   *
 * (c)2026 SWAT                    *
 * http://www.dc-swat.ru           *
 **********************************/

#include "ds.h"
#include <tsunami/tsunami.h>

#define APP_TSU_BODY_TYPE "tsunami"
#define APP_TSU_COLOR_MAX 255.0f
#define APP_TSU_DEFAULT_ALPHA 1.0f
#define APP_TSU_DEFAULT_LABEL_SIZE 16
#define APP_TSU_DEFAULT_Z "1.0"
#define APP_TSU_DEFAULT_CARDSTACK_FADE "0.12"

enum {
	APP_TSU_DRAWABLE_LABEL = 1,
	APP_TSU_DRAWABLE_BANNER,
	APP_TSU_DRAWABLE_RECTANGLE,
	APP_TSU_DRAWABLE_GRADIENT,
	APP_TSU_DRAWABLE_CHECKBOX,
	APP_TSU_DRAWABLE_TEXTBOX,
	APP_TSU_DRAWABLE_CIRCLE,
	APP_TSU_DRAWABLE_WAVE,
	APP_TSU_DRAWABLE_BOX,
	APP_TSU_DRAWABLE_TRIANGLE,
	APP_TSU_DRAWABLE_CARDSTACK
};

static Scene *appTsunamiParseScene = NULL;

static int parseAppTsunamiElement(App_t *app, mxml_node_t *node, SDL_Rect *parent);
static int parseAppTsunamiPanelElement(App_t *app, mxml_node_t *node, SDL_Rect *parent);

static int parseAppTsunamiBool(mxml_node_t *node, char *name, int defValue) {

	char *value = FindXmlAttr(name, node, NULL);

	if(value == NULL) {
		return defValue;
	}

	if(!strncasecmp(value, "true", 4) || !strncasecmp(value, "yes", 3) || !strncmp(value, "1", 1)) {
		return 1;
	}

	return 0;
}

static pvr_list_type_t parseAppTsunamiList(mxml_node_t *node, pvr_list_type_t defValue) {

	char *trans = FindXmlAttr("transparent", node, NULL);

	if(trans != NULL) {
		if(!strcasecmp(trans, "true") || !strcasecmp(trans, "yes") || atof(trans) > 0.0f) {
			return PVR_LIST_TR_POLY;
		}
		else if(!strcasecmp(trans, "false") || !strcasecmp(trans, "no") || atof(trans) == 0.0f) {
			return PVR_LIST_OP_POLY;
		}
	}

	return defValue;
}

static void parseAppTsunamiColor(char *value, Color *color, Color *defColor) {

	unsigned int r = 255, g = 255, b = 255, a = 255;
	int cnt;

	if(defColor != NULL) {
		*color = *defColor;
	}

	if(value == NULL || *value != '#') {
		return;
	}

	cnt = sscanf(value, "#%02x%02x%02x%02x", &r, &g, &b, &a);

	if(cnt >= 3) {
		color->r = (float)r / APP_TSU_COLOR_MAX;
		color->g = (float)g / APP_TSU_COLOR_MAX;
		color->b = (float)b / APP_TSU_COLOR_MAX;
		color->a = (float)a / APP_TSU_COLOR_MAX;
	}
}

static uint32 parseAppTsunamiGradientColor(char *value, uint32 defValue) {

	unsigned int r = 0, g = 0, b = 0, a = 255;
	int cnt;

	if(value == NULL || *value != '#') {
		return defValue;
	}

	cnt = sscanf(value, "#%02x%02x%02x%02x", &r, &g, &b, &a);

	if(cnt < 3) {
		return defValue;
	}

	return (a << 24) | (r << 16) | (g << 8) | b;
}

int isTsunamiBody(mxml_node_t *node) {

	char *type = FindXmlAttr("type", node, NULL);
	if(type && (!strncasecmp(type, APP_TSU_BODY_TYPE, strlen(APP_TSU_BODY_TYPE)))) {
		return 1;
	}

	return 0;
}

int isTsunamiApp(App_t *app) {

	mxml_node_t *node;

	if(app == NULL || app->xml == NULL) {
		return 0;
	}

	node = mxmlFindElement(app->xml, app->xml, "body", NULL, NULL, MXML_DESCEND);
	return node != NULL ? isTsunamiBody(node) : 0;
}

static Texture *getAppTsunamiTexture(App_t *app, mxml_node_t *node, char *name) {

	Item_t *item;
	Texture *texture;
	char file[NAME_MAX];
	int use_alpha;
	int yflip;
	unsigned int flags;

	if(name == NULL) {
		return NULL;
	}

	item = listGetItemByNameAndType(app->resources, name, LIST_ITEM_TSU_IMAGE);

	if(item != NULL) {
		return (Texture *)item->data;
	}

	relativeFilePath_wb(file, app->fn, name);
	use_alpha = parseAppTsunamiBool(node, "alpha", 1);
	yflip = parseAppTsunamiBool(node, "yflip", 0);
	flags = atoi(FindXmlAttr("flags", node, "0"));
	texture = TSU_TextureCreateFromFile(file, use_alpha, yflip, flags);

	if(texture != NULL) {
		listAddItem(app->resources, LIST_ITEM_TSU_IMAGE, name, (void *)texture, 0);
	}

	return texture;
}

static Font *getAppTsunamiFont(App_t *app, mxml_node_t *node, char *name) {

	Item_t *item;
	Font *font;
	char file[NAME_MAX];

	if(name == NULL) {
		return NULL;
	}

	item = listGetItemByNameAndType(app->resources, name, LIST_ITEM_TSU_FONT);

	if(item != NULL) {
		return (Font *)item->data;
	}

	relativeFilePath_wb(file, app->fn, name);
	font = TSU_FontCreate(file, parseAppTsunamiList(node, PVR_LIST_TR_POLY));

	if(font != NULL) {
		listAddItem(app->resources, LIST_ITEM_TSU_FONT, name, (void *)font, 0);
	}

	return font;
}

int appTsunamiParseImageResource(App_t *app, mxml_node_t *node, char *name, char *src) {

	Texture *texture;
	int use_alpha;
	int yflip;
	unsigned int flags;

	if(name == NULL) {
		name = FindXmlAttr("src", node, NULL);
	}

	if(src == NULL || src[0] == '\0') {
		ds_printf("DS_ERROR: Empty src attribute in TSU image resource '%s'\n", name);
		return 0;
	}

	use_alpha = parseAppTsunamiBool(node, "alpha", 1);
	yflip = parseAppTsunamiBool(node, "yflip", 0);
	flags = atoi(FindXmlAttr("flags", node, "0"));
	texture = TSU_TextureCreateFromFile(src, use_alpha, yflip, flags);

	if(texture == NULL) {
		ds_printf("DS_ERROR: Can't load image %s\n", src);
		return 0;
	}

	listAddItem(app->resources, LIST_ITEM_TSU_IMAGE, name, (void *)texture, 0);
	return 1;
}

int appTsunamiParseFontResource(App_t *app, mxml_node_t *node, char *name, char *src) {

	Font *font;

	if(name == NULL) {
		name = FindXmlAttr("src", node, NULL);
	}

	font = TSU_FontCreate(src, parseAppTsunamiList(node, PVR_LIST_TR_POLY));

	if(font == NULL) {
		ds_printf("DS_ERROR: Can't load font %s\n", src);
		return 0;
	}

	listAddItem(app->resources, LIST_ITEM_TSU_FONT, name, (void *)font, 0);
	return 1;
}

static void parseAppTsunamiAlign(mxml_node_t *node, Drawable *drawable, int w, int h, SDL_Rect *parent) {

	char *align = FindXmlAttr("align", node, NULL);
	char *valign = FindXmlAttr("valign", node, NULL);
	const Vector *cur = TSU_DrawableGetTranslate(drawable);
	Vector pos;

	if(cur == NULL) {
		return;
	}

	pos = *cur;

	if(align != NULL) {
		if(!strncmp(align, "center", 6)) {
			pos.x = parent->x + ((parent->w - w) / 2.0f);
		}
		else if(!strncmp(align, "right", 5)) {
			pos.x = parent->x + parent->w - w;
		}
	}

	if(valign != NULL) {
		if(!strncmp(valign, "center", 6)) {
			pos.y = parent->y + ((parent->h - h) / 2.0f);
		}
		else if(!strncmp(valign, "bottom", 6)) {
			pos.y = parent->y + parent->h - h;
		}
	}

	TSU_DrawableSetTranslate(drawable, &pos);
}

static void parseAppTsunamiDrawableAttrs(mxml_node_t *node, Drawable *drawable, SDL_Rect *parent) {

	int x, y, w, h;
	char *alpha = FindXmlAttr("alpha", node, NULL);
	char *transparent = FindXmlAttr("transparent", node, NULL);
	char *visibility = FindXmlAttr("visibility", node, NULL);
	char *scale = FindXmlAttr("scale", node, NULL);
	char *scale_x = FindXmlAttr("scaleX", node, NULL);
	char *scale_y = FindXmlAttr("scaleY", node, NULL);
	Vector pos;

	parseNodeSize(node, parent, &w, &h);
	parseNodePosition(node, parent, &x, &y);

	pos.x = (float)(parent->x + x);
	pos.y = (float)(parent->y + y);
	pos.z = atof(FindXmlAttr("z", node, APP_TSU_DEFAULT_Z));
	pos.w = 1.0f;
	TSU_DrawableSetTranslate(drawable, &pos);
	parseAppTsunamiAlign(node, drawable, w, h, parent);

	if(alpha != NULL) {
		TSU_DrawableSetAlpha(drawable, atof(alpha));
	}

	if(transparent != NULL) {
		TSU_DrawableSetAlpha(drawable, 1.0f - atof(transparent));
	}

	if(visibility != NULL && (!strncmp(visibility, "false", 5) || !strncmp(visibility, "hidden", 6))) {
		TSU_DrawableSetAlpha(drawable, 0.0f);
	}

	if(scale != NULL || scale_x != NULL || scale_y != NULL) {
		Vector s = {
			scale_x ? atof(scale_x) : scale ? atof(scale) : 1.0f,
			scale_y ? atof(scale_y) : scale ? atof(scale) : 1.0f,
			1.0f,
			1.0f
		};
		TSU_DrawableSetScale(drawable, &s);
	}
}

static int addAppTsunamiElement(App_t *app, char *name, void *object, int type) {

	Scene *target_scene;

	if(object == NULL) {
		return 0;
	}

	if(listAddItem(app->elements, LIST_ITEM_TSU_DRAWABLE, name, object, type) == NULL) {
		return 0;
	}

	target_scene = appTsunamiParseScene;

	if(target_scene != NULL) {
		TSU_DrawableSubAdd((Drawable *)target_scene, (Drawable *)object);
		return 1;
	}

	switch(type) {
		case APP_TSU_DRAWABLE_LABEL:
			TSU_AppSubAddLabel(app->tsunami, (Label *)object);
			break;
		case APP_TSU_DRAWABLE_BANNER:
			TSU_AppSubAddBanner(app->tsunami, (Banner *)object);
			break;
		case APP_TSU_DRAWABLE_RECTANGLE:
			TSU_AppSubAddRectangle(app->tsunami, (Rectangle *)object);
			break;
		case APP_TSU_DRAWABLE_GRADIENT:
			TSU_AppSubAddGradient(app->tsunami, (Gradient *)object);
			break;
		case APP_TSU_DRAWABLE_CIRCLE:
			TSU_AppSubAddCircle(app->tsunami, (Circle *)object);
			break;
		case APP_TSU_DRAWABLE_WAVE:
			TSU_AppSubAddWave(app->tsunami, (Wave *)object);
			break;
		case APP_TSU_DRAWABLE_BOX:
			TSU_AppSubAddBox(app->tsunami, (Box *)object);
			break;
		case APP_TSU_DRAWABLE_CHECKBOX:
		case APP_TSU_DRAWABLE_TEXTBOX:
		case APP_TSU_DRAWABLE_TRIANGLE:
		case APP_TSU_DRAWABLE_CARDSTACK:
			TSU_DrawableSubAdd((Drawable *)TSU_AppGetScene(app->tsunami), (Drawable *)object);
			break;
		default:
			return 0;
	}

	return 1;
}

static int parseAppTsunamiLabelElement(App_t *app, mxml_node_t *node, SDL_Rect *parent) {

	Font *font;
	Label *label;
	Color color = {APP_TSU_DEFAULT_ALPHA, 1.0f, 1.0f, 1.0f};
	char *name = FindXmlAttr("name", node, node->value.element.name);
	char *font_name = FindXmlAttr("font", node, NULL);
	int font_size = atoi(FindXmlAttr("fontsize", node, FindXmlAttr("size", node, "16")));
	float label_w, label_h;

	font = getAppTsunamiFont(app, node, font_name);

	if(font == NULL) {
		ds_printf("DS_ERROR: Can't find TSU font '%s'\n", font_name);
		return 0;
	}

	if(font_size <= 0) {
		font_size = APP_TSU_DEFAULT_LABEL_SIZE;
	}

	label = TSU_LabelCreate(font,
		FindXmlAttr("text", node, ""),
		font_size,
		parseAppTsunamiBool(node, "centered", 0),
		parseAppTsunamiBool(node, "smear", 1),
		parseAppTsunamiBool(node, "fixWidth", 0));

	if(label == NULL) {
		return 0;
	}

	parseAppTsunamiColor(FindXmlAttr("color", node, FindXmlAttr("fontcolor", node, NULL)), &color, &color);
	TSU_LabelSetTint(label, &color);
	parseAppTsunamiDrawableAttrs(node, (Drawable *)label, parent);
	TSU_LabelGetSize(label, &label_w, &label_h);
	parseAppTsunamiAlign(node, (Drawable *)label, (int)label_w, (int)label_h, parent);

	return addAppTsunamiElement(app, name, (void *)label, APP_TSU_DRAWABLE_LABEL);
}

static int parseAppTsunamiImageElement(App_t *app, mxml_node_t *node, SDL_Rect *parent) {

	Texture *texture;
	Banner *banner;
	Color tint = {APP_TSU_DEFAULT_ALPHA, 1.0f, 1.0f, 1.0f};
	int x, y, w, h;
	float z;
	Vector pos;
	char *name = FindXmlAttr("name", node, node->value.element.name);
	char *image_name = FindXmlAttr("src", node,
		FindXmlAttr("normal", node,
		FindXmlAttr("background", node, NULL)));
	char *visibility = FindXmlAttr("visibility", node, NULL);

	texture = getAppTsunamiTexture(app, node, image_name);

	if(texture == NULL) {
		ds_printf("DS_ERROR: Can't find TSU image '%s'\n", image_name);
		return 0;
	}

	parseNodeSize(node, parent, &w, &h);
	parseNodePosition(node, parent, &x, &y);
	z = atof(FindXmlAttr("z", node, APP_TSU_DEFAULT_Z));
	banner = TSU_BannerCreate(parseAppTsunamiList(node, PVR_LIST_TR_POLY), texture);

	if(banner == NULL) {
		return 0;
	}

	TSU_BannerSetSize(banner, (float)w, (float)h);

	pos.x = (float)(parent->x + x) + ((float)w / 2.0f);
	pos.y = (float)(parent->y + y) + ((float)h / 2.0f);
	pos.z = z;
	pos.w = 1.0f;
	TSU_DrawableSetTranslate((Drawable *)banner, &pos);
	parseAppTsunamiColor(FindXmlAttr("tint", node, NULL), &tint, &tint);
	TSU_DrawableSetTint((Drawable *)banner, &tint);
	parseAppTsunamiAlign(node, (Drawable *)banner, w, h, parent);

	if(FindXmlAttr("alpha", node, NULL) != NULL) {
		TSU_DrawableSetAlpha((Drawable *)banner, atof(FindXmlAttr("alpha", node, "1.0")));
	}
	else if(FindXmlAttr("transparent", node, NULL) != NULL) {
		TSU_DrawableSetAlpha((Drawable *)banner, 1.0f - atof(FindXmlAttr("transparent", node, "0.0")));
	}

	if(visibility != NULL && (!strncmp(visibility, "false", 5) || !strncmp(visibility, "hidden", 6))) {
		TSU_DrawableSetAlpha((Drawable *)banner, 0.0f);
	}

	return addAppTsunamiElement(app, name, (void *)banner, APP_TSU_DRAWABLE_BANNER);
}

static int parseAppTsunamiRectangleElement(App_t *app, mxml_node_t *node, SDL_Rect *parent) {

	Rectangle *rect;
	Color color = {APP_TSU_DEFAULT_ALPHA, 1.0f, 1.0f, 1.0f};
	Color border_color = {APP_TSU_DEFAULT_ALPHA, 1.0f, 1.0f, 1.0f};
	int x, y, w, h;
	float z;
	float radius;
	float border_width;
	char *name = FindXmlAttr("name", node, node->value.element.name);

	parseNodeSize(node, parent, &w, &h);
	parseNodePosition(node, parent, &x, &y);
	z = atof(FindXmlAttr("z", node, APP_TSU_DEFAULT_Z));
	radius = atof(FindXmlAttr("radius", node, "0.0"));
	border_width = atof(FindXmlAttr("borderWidth", node, FindXmlAttr("border", node, "0.0")));
	parseAppTsunamiColor(FindXmlAttr("color", node, FindXmlAttr("background", node, NULL)), &color, &color);
	parseAppTsunamiColor(FindXmlAttr("borderColor", node, NULL), &border_color, &border_color);

	if(border_width > 0.0f) {
		rect = TSU_RectangleCreateWithBorder(parseAppTsunamiList(node, PVR_LIST_OP_POLY),
			(float)(parent->x + x),
			(float)(parent->y + y + h),
			(float)w,
			(float)h,
			&color,
			z,
			border_width,
			&border_color,
			radius);
	}
	else {
		rect = TSU_RectangleCreate(parseAppTsunamiList(node, PVR_LIST_OP_POLY),
			(float)(parent->x + x),
			(float)(parent->y + y + h),
			(float)w,
			(float)h,
			&color,
			z,
			radius);
	}

	return addAppTsunamiElement(app, name, (void *)rect, APP_TSU_DRAWABLE_RECTANGLE);
}

static int parseAppTsunamiGradientElement(App_t *app, mxml_node_t *node, SDL_Rect *parent) {

	Gradient *gradient;
	int x, y, w, h;
	float z;
	Vector pos;
	uint32 tl, tr, br, bl;
	char *name = FindXmlAttr("name", node, node->value.element.name);

	parseNodeSize(node, parent, &w, &h);
	parseNodePosition(node, parent, &x, &y);
	z = atof(FindXmlAttr("z", node, "0.0"));
	gradient = TSU_GradientCreate(parseAppTsunamiList(node, PVR_LIST_OP_POLY), (float)w, (float)h, z);

	if(gradient == NULL) {
		return 0;
	}

	tl = parseAppTsunamiGradientColor(FindXmlAttr("topLeft", node, FindXmlAttr("color1", node, NULL)), 0xFFFFFFFF);
	tr = parseAppTsunamiGradientColor(FindXmlAttr("topRight", node, FindXmlAttr("color2", node, NULL)), tl);
	br = parseAppTsunamiGradientColor(FindXmlAttr("bottomRight", node, FindXmlAttr("color3", node, NULL)), tr);
	bl = parseAppTsunamiGradientColor(FindXmlAttr("bottomLeft", node, FindXmlAttr("color4", node, NULL)), br);
	TSU_GradientSetColors(gradient, tl, tr, br, bl);

	pos.x = (float)(parent->x + x);
	pos.y = (float)(parent->y + y + h);
	pos.z = z;
	pos.w = 1.0f;
	TSU_DrawableSetTranslate((Drawable *)gradient, &pos);

	return addAppTsunamiElement(app, name, (void *)gradient, APP_TSU_DRAWABLE_GRADIENT);
}

static int parseAppTsunamiCheckBoxElement(App_t *app, mxml_node_t *node, SDL_Rect *parent) {

	Font *font;
	CheckBox *checkbox;
	Color color = {APP_TSU_DEFAULT_ALPHA, 1.0f, 1.0f, 1.0f};
	char *name = FindXmlAttr("name", node, node->value.element.name);
	char *font_name = FindXmlAttr("font", node, NULL);
	int font_size = atoi(FindXmlAttr("fontsize", node, FindXmlAttr("size", node, "16")));
	int w, h;
	char *on_text = FindXmlAttr("onText", node, NULL);
	char *off_text = FindXmlAttr("offText", node, NULL);
	char *checked = FindXmlAttr("checked", node, NULL);

	font = getAppTsunamiFont(app, node, font_name);

	if(font == NULL) {
		ds_printf("DS_ERROR: Can't find TSU font '%s'\n", font_name);
		return 0;
	}

	if(font_size <= 0) {
		font_size = APP_TSU_DEFAULT_LABEL_SIZE;
	}

	parseNodeSize(node, parent, &w, &h);
	parseAppTsunamiColor(FindXmlAttr("color", node, FindXmlAttr("fontcolor", node, NULL)), &color, &color);

	if(on_text != NULL && off_text != NULL) {
		checkbox = TSU_CheckBoxCreateWithCustomText(font, font_size, (float)w, (float)h, &color, on_text, off_text);
	}
	else {
		checkbox = TSU_CheckBoxCreate(font, font_size, (float)w, (float)h, &color);
	}

	if(checkbox == NULL) {
		return 0;
	}

	if(checked != NULL && (!strcasecmp(checked, "true") || !strcasecmp(checked, "yes") || !strcmp(checked, "1"))) {
		TSU_CheckBoxSetOn(checkbox);
	}
	else {
		TSU_CheckBoxSetOff(checkbox);
	}

	parseAppTsunamiDrawableAttrs(node, (Drawable *)checkbox, parent);

	return addAppTsunamiElement(app, name, (void *)checkbox, APP_TSU_DRAWABLE_CHECKBOX);
}

static int parseAppTsunamiTextBoxElement(App_t *app, mxml_node_t *node, SDL_Rect *parent) {

	Font *font;
	TextBox *textbox;
	Color color = {APP_TSU_DEFAULT_ALPHA, 1.0f, 1.0f, 1.0f};
	char *name = FindXmlAttr("name", node, node->value.element.name);
	char *font_name = FindXmlAttr("font", node, NULL);
	int font_size = atoi(FindXmlAttr("fontsize", node, FindXmlAttr("size", node, "16")));
	int w, h;
	char *value = FindXmlAttr("value", node, NULL);

	font = getAppTsunamiFont(app, node, font_name);

	if(font == NULL) {
		ds_printf("DS_ERROR: Can't find TSU font '%s'\n", font_name);
		return 0;
	}

	if(font_size <= 0) {
		font_size = APP_TSU_DEFAULT_LABEL_SIZE;
	}

	parseNodeSize(node, parent, &w, &h);
	parseAppTsunamiColor(FindXmlAttr("color", node, FindXmlAttr("fontcolor", node, NULL)), &color, &color);

	textbox = TSU_TextBoxCreate(font,
		font_size,
		parseAppTsunamiBool(node, "centered", 0),
		(float)w,
		(float)h,
		&color,
		parseAppTsunamiBool(node, "letters", 1),
		parseAppTsunamiBool(node, "capLetters", 1),
		parseAppTsunamiBool(node, "numbers", 1),
		parseAppTsunamiBool(node, "symbols", 1));

	if(textbox == NULL) {
		return 0;
	}

	if(value != NULL) {
		TSU_TextBoxSetText(textbox, value);
	}

	parseAppTsunamiDrawableAttrs(node, (Drawable *)textbox, parent);

	return addAppTsunamiElement(app, name, (void *)textbox, APP_TSU_DRAWABLE_TEXTBOX);
}

static int parseAppTsunamiCircleElement(App_t *app, mxml_node_t *node, SDL_Rect *parent) {

	Circle *circle;
	Color center_color = {APP_TSU_DEFAULT_ALPHA, 1.0f, 1.0f, 1.0f};
	Color edge_color = {APP_TSU_DEFAULT_ALPHA, 1.0f, 1.0f, 1.0f};
	char *name = FindXmlAttr("name", node, node->value.element.name);
	float radius = atof(FindXmlAttr("radius", node, "128.0"));
	int points = atoi(FindXmlAttr("points", node, "16"));

	parseAppTsunamiColor(FindXmlAttr("centerColor", node, FindXmlAttr("color", node, NULL)), &center_color, &center_color);
	parseAppTsunamiColor(FindXmlAttr("edgeColor", node, NULL), &edge_color, &center_color);

	circle = TSU_CircleCreate(parseAppTsunamiList(node, PVR_LIST_OP_POLY), radius, points, &center_color, &edge_color);

	if(circle == NULL) {
		return 0;
	}

	parseAppTsunamiDrawableAttrs(node, (Drawable *)circle, parent);

	return addAppTsunamiElement(app, name, (void *)circle, APP_TSU_DRAWABLE_CIRCLE);
}

static int parseAppTsunamiWaveElement(App_t *app, mxml_node_t *node, SDL_Rect *parent) {

	Wave *wave;
	char *name = FindXmlAttr("name", node, node->value.element.name);
	int w, h;
	float freq = atof(FindXmlAttr("freq", node, "0.8"));
	float amp = atof(FindXmlAttr("amp", node, "13.0"));
	float speed = atof(FindXmlAttr("speed", node, "0.02"));
	int segs = atoi(FindXmlAttr("segs", node, "32"));
	char *color1_str = FindXmlAttr("color", node, FindXmlAttr("color1", node, "#FF0000E5"));
	char *color2_str = FindXmlAttr("color2", node, "#70C9DB66");

	wave = TSU_WaveCreate(parseAppTsunamiList(node, PVR_LIST_OP_POLY));

	if(wave == NULL) {
		return 0;
	}

	parseNodeSize(node, parent, &w, &h);
	TSU_WaveSetSize(wave, (float)w, (float)h);
	TSU_WaveSetParams(wave, freq, amp, speed, segs);

	parseAppTsunamiDrawableAttrs(node, (Drawable *)wave, parent);

	if(color1_str != NULL && color2_str != NULL) {
		Color c1 = {APP_TSU_DEFAULT_ALPHA, 1.0f, 1.0f, 1.0f};
		Color c2 = {APP_TSU_DEFAULT_ALPHA, 1.0f, 1.0f, 1.0f};
		uint32 duration = atoi(FindXmlAttr("duration", node, "10000"));
		uint32 hold_duration = atoi(FindXmlAttr("hold", node, "5000"));

		parseAppTsunamiColor(color1_str, &c1, &c1);
		parseAppTsunamiColor(color2_str, &c2, &c2);
		TSU_WaveSetColorAnimation(wave, c1.r, c1.g, c1.b, c1.a, c2.r, c2.g, c2.b, c2.a, duration, hold_duration);
	}
	else if(color1_str != NULL) {
		Color c1 = {APP_TSU_DEFAULT_ALPHA, 1.0f, 1.0f, 1.0f};
		parseAppTsunamiColor(color1_str, &c1, &c1);
		TSU_WaveSetColor(wave, c1.r, c1.g, c1.b, c1.a);
	}

	return addAppTsunamiElement(app, name, (void *)wave, APP_TSU_DRAWABLE_WAVE);
}

static int parseAppTsunamiBoxElement(App_t *app, mxml_node_t *node, SDL_Rect *parent) {

	Box *box;
	Color color = {APP_TSU_DEFAULT_ALPHA, 1.0f, 1.0f, 1.0f};
	int x, y, w, h;
	float z;
	float radius;
	float border_width;
	char *name = FindXmlAttr("name", node, node->value.element.name);

	parseNodeSize(node, parent, &w, &h);
	parseNodePosition(node, parent, &x, &y);
	z = atof(FindXmlAttr("z", node, APP_TSU_DEFAULT_Z));
	radius = atof(FindXmlAttr("radius", node, "0.0"));
	border_width = atof(FindXmlAttr("borderWidth", node, FindXmlAttr("border", node, "1.0")));
	parseAppTsunamiColor(FindXmlAttr("color", node, NULL), &color, &color);

	box = TSU_BoxCreate(parseAppTsunamiList(node, PVR_LIST_OP_POLY),
		(float)(parent->x + x),
		(float)(parent->y + y + h),
		(float)w,
		(float)h,
		border_width,
		&color,
		z,
		radius);

	if(box == NULL) {
		return 0;
	}

	parseAppTsunamiDrawableAttrs(node, (Drawable *)box, parent);

	return addAppTsunamiElement(app, name, (void *)box, APP_TSU_DRAWABLE_BOX);
}

static int parseAppTsunamiTriangleElement(App_t *app, mxml_node_t *node, SDL_Rect *parent) {

	Triangle *triangle;
	Color color = {APP_TSU_DEFAULT_ALPHA, 1.0f, 1.0f, 1.0f};
	Color border_color = {APP_TSU_DEFAULT_ALPHA, 1.0f, 1.0f, 1.0f};
	float x1, y1, x2, y2, x3, y3;
	float z;
	float radius;
	float border_width;
	char *name = FindXmlAttr("name", node, node->value.element.name);

	x1 = parent->x + atof(FindXmlAttr("x1", node, "0.0"));
	y1 = parent->y + atof(FindXmlAttr("y1", node, "0.0"));
	x2 = parent->x + atof(FindXmlAttr("x2", node, "0.0"));
	y2 = parent->y + atof(FindXmlAttr("y2", node, "0.0"));
	x3 = parent->x + atof(FindXmlAttr("x3", node, "0.0"));
	y3 = parent->y + atof(FindXmlAttr("y3", node, "0.0"));

	z = atof(FindXmlAttr("z", node, APP_TSU_DEFAULT_Z));
	radius = atof(FindXmlAttr("radius", node, "0.0"));
	border_width = atof(FindXmlAttr("borderWidth", node, FindXmlAttr("border", node, "0.0")));
	parseAppTsunamiColor(FindXmlAttr("color", node, NULL), &color, &color);
	parseAppTsunamiColor(FindXmlAttr("borderColor", node, NULL), &border_color, &border_color);

	if(border_width > 0.0f) {
		triangle = TSU_TriangleCreateWithBorder(parseAppTsunamiList(node, PVR_LIST_OP_POLY),
			x1, y1, x2, y2, x3, y3,
			&color,
			z,
			border_width,
			&border_color,
			radius);
	}
	else {
		triangle = TSU_TriangleCreate(parseAppTsunamiList(node, PVR_LIST_OP_POLY),
			x1, y1, x2, y2, x3, y3,
			&color,
			z,
			radius);
	}

	if(triangle == NULL) {
		return 0;
	}

	parseAppTsunamiDrawableAttrs(node, (Drawable *)triangle, parent);

	return addAppTsunamiElement(app, name, (void *)triangle, APP_TSU_DRAWABLE_TRIANGLE);
}

static int parseAppTsunamiCardStackElement(App_t *app, mxml_node_t *node, SDL_Rect *parent) {

	CardStack *stack;
	mxml_node_t *n;
	SDL_Rect area;
	int x, y, w, h;
	int index;
	char *name = FindXmlAttr("name", node, node->value.element.name);
	char *back = FindXmlAttr("background", node, NULL);

	parseNodeSize(node, parent, &w, &h);
	parseNodePosition(node, parent, &x, &y);
	area.x = parent->x + x;
	area.y = parent->y + y;
	area.w = w;
	area.h = h;

	stack = TSU_CardStackCreate((float)w, (float)h);

	if(stack == NULL) {
		return 0;
	}

	parseAppTsunamiDrawableAttrs(node, (Drawable *)stack, parent);
	TSU_CardStackSetFadeStep(stack, atof(FindXmlAttr("fade", node,
		FindXmlAttr("transition", node, APP_TSU_DEFAULT_CARDSTACK_FADE))));

	if(back != NULL && *back == '#') {
		Color color = {APP_TSU_DEFAULT_ALPHA, 0.0f, 0.0f, 0.0f};
		float z = atof(FindXmlAttr("z", node, APP_TSU_DEFAULT_Z));

		parseAppTsunamiColor(back, &color, &color);
		TSU_CardStackSetBackgroundColor(stack, &color, parseAppTsunamiList(node, PVR_LIST_OP_POLY), z);
	}
	else if(back != NULL) {
		Texture *texture = getAppTsunamiTexture(app, node, back);
		Banner *banner;
		Vector pos;
		float z;

		if(texture == NULL) {
			ds_printf("DS_ERROR: Can't find TSU image '%s'\n", back);
			TSU_CardStackDestroy(&stack);
			return 0;
		}

		z = atof(FindXmlAttr("z", node, APP_TSU_DEFAULT_Z));
		banner = TSU_BannerCreate(parseAppTsunamiList(node, PVR_LIST_TR_POLY), texture);

		if(banner == NULL) {
			TSU_CardStackDestroy(&stack);
			return 0;
		}

		TSU_BannerSetSize(banner, (float)w, (float)h);
		pos.x = (float)area.x + ((float)w / 2.0f);
		pos.y = (float)area.y + ((float)h / 2.0f);
		pos.z = z;
		pos.w = 1.0f;
		TSU_DrawableSetTranslate((Drawable *)banner, &pos);
		TSU_CardStackSetBackground(stack, banner);
	}

	if(node->child && node->child->next) {
		n = node->child->next;

		while(n) {
			if(n->type == MXML_ELEMENT && n->value.element.name[0] != '!') {
				Scene *card = TSU_SceneCreate();
				char *card_name = FindXmlAttr("name", n, n->value.element.name);
				int parsed = 0;

				if(card == NULL) {
					TSU_CardStackDestroy(&stack);
					return 0;
				}

				appTsunamiParseScene = card;
				parsed = parseAppTsunamiElement(app, n, &area);
				appTsunamiParseScene = NULL;

				if(!parsed) {
					TSU_SceneDestroy(&card);
					TSU_CardStackDestroy(&stack);
					return 0;
				}

				TSU_CardStackAddCard(stack, card, card_name);
			}

			n = n->next;
		}
	}

	index = atoi(FindXmlAttr("index", node, "0"));
	TSU_CardStackSetIndex(stack, index);

	return addAppTsunamiElement(app, name, (void *)stack, APP_TSU_DRAWABLE_CARDSTACK);
}

static int parseAppTsunamiPanelElement(App_t *app, mxml_node_t *node, SDL_Rect *parent) {

	mxml_node_t *n;
	SDL_Rect area;
	int x, y, w, h;
	char *back;

	parseNodeSize(node, parent, &w, &h);
	parseNodePosition(node, parent, &x, &y);
	area.x = parent->x + x;
	area.y = parent->y + y;
	area.w = w;
	area.h = h;
	back = FindXmlAttr("background", node, NULL);

	if(back != NULL && *back == '#') {
		if(!parseAppTsunamiRectangleElement(app, node, parent)) {
			return 0;
		}
	}
	else if(back != NULL) {
		if(!parseAppTsunamiImageElement(app, node, parent)) {
			return 0;
		}
	}

	if(node->child && node->child->next) {
		n = node->child->next;

		while(n) {
			if(n->type == MXML_ELEMENT && n->value.element.name[0] != '!') {
				if(!parseAppTsunamiElement(app, n, &area)) {
					return 0;
				}
			}

			n = n->next;
		}
	}

	return 1;
}

static int parseAppTsunamiElement(App_t *app, mxml_node_t *node, SDL_Rect *parent) {

	if(!strncmp(node->value.element.name, "label", 5)) {
		return parseAppTsunamiLabelElement(app, node, parent);
	}
	else if(!strncmp(node->value.element.name, "image", 5)) {
		return parseAppTsunamiImageElement(app, node, parent);
	}
	else if(!strncmp(node->value.element.name, "rectangle", 9) || !strncmp(node->value.element.name, "rect", 4)) {
		return parseAppTsunamiRectangleElement(app, node, parent);
	}
	else if(!strncmp(node->value.element.name, "gradient", 8)) {
		return parseAppTsunamiGradientElement(app, node, parent);
	}
	else if(!strncmp(node->value.element.name, "panel", 5)) {
		return parseAppTsunamiPanelElement(app, node, parent);
	}
	else if(!strncmp(node->value.element.name, "cardstack", 9)) {
		return parseAppTsunamiCardStackElement(app, node, parent);
	}
	else if(!strncmp(node->value.element.name, "circle", 6)) {
		return parseAppTsunamiCircleElement(app, node, parent);
	}
	else if(!strncmp(node->value.element.name, "wave", 4)) {
		return parseAppTsunamiWaveElement(app, node, parent);
	}
	else if(!strncmp(node->value.element.name, "box", 3)) {
		return parseAppTsunamiBoxElement(app, node, parent);
	}
	else if(!strncmp(node->value.element.name, "triangle", 8)) {
		return parseAppTsunamiTriangleElement(app, node, parent);
	}
	else if(!strncmp(node->value.element.name, "input", 5)) {
		char *type = FindXmlAttr("type", node, "unknown");

		if(!strncmp(type, "button", 6)) {
			return parseAppTsunamiImageElement(app, node, parent);
		}
		else if(!strncmp(type, "checkbox", 8)) {
			return parseAppTsunamiCheckBoxElement(app, node, parent);
		}
		else if(!strncmp(type, "text", 4)) {
			return parseAppTsunamiTextBoxElement(app, node, parent);
		}
	}

	ds_printf("DS_ERROR: Unknown TSU element - %s\n", node->value.element.name);
	return 0;
}

int appTsunamiBuildBody(App_t *app, mxml_node_t *node) {

	mxml_node_t *n;
	SDL_Rect body_area;
	int x, y, w, h;
	char *bg;

	if(app->tsunami == NULL) {
		if((app->tsunami = TSU_AppCreate(NULL)) == NULL) {
			return 0;
		}
	}

	parseNodeSize(node, NULL, &w, &h);
	parseNodePosition(node, NULL, &x, &y);
	body_area.x = x;
	body_area.y = y;
	body_area.w = w;
	body_area.h = h;
	bg = FindXmlAttr("backgroundColor", node, FindXmlAttr("background", node, NULL));

	if(bg != NULL && *bg == '#') {
		Color color = {1.0f, 0.0f, 0.0f, 0.0f};
		parseAppTsunamiColor(bg, &color, &color);
		TSU_AppSetBg(app->tsunami, color.r, color.g, color.b);
	}
	else if(bg != NULL) {
		if(!parseAppTsunamiImageElement(app, node, &body_area)) {
			return 0;
		}
	}

	if(node->child && node->child->next) {
		n = node->child->next;

		while(n) {
			if(n->type == MXML_ELEMENT && n->value.element.name[0] != '!') {
				if(!parseAppTsunamiElement(app, n, &body_area)) {
					return 0;
				}
			}

			n = n->next;
		}
	}

	return 1;
}

void appTsunamiDestroyElement(App_t *app, void *object, int type) {

	if(object == NULL) {
		return;
	}

	if(app != NULL && app->tsunami != NULL) {
		switch(type) {
			case APP_TSU_DRAWABLE_LABEL:
				TSU_AppSubRemoveLabel(app->tsunami, (Label *)object);
				break;
			case APP_TSU_DRAWABLE_BANNER:
				TSU_AppSubRemoveBanner(app->tsunami, (Banner *)object);
				break;
			case APP_TSU_DRAWABLE_RECTANGLE:
				TSU_AppSubRemoveRectangle(app->tsunami, (Rectangle *)object);
				break;
			case APP_TSU_DRAWABLE_GRADIENT:
				TSU_AppSubRemoveGradient(app->tsunami, (Gradient *)object);
				break;
			case APP_TSU_DRAWABLE_CIRCLE:
				TSU_AppSubRemoveCircle(app->tsunami, (Circle *)object);
				break;
			case APP_TSU_DRAWABLE_WAVE:
				TSU_AppSubRemoveWave(app->tsunami, (Wave *)object);
				break;
			case APP_TSU_DRAWABLE_BOX:
				TSU_AppSubRemoveBox(app->tsunami, (Box *)object);
				break;
			case APP_TSU_DRAWABLE_CHECKBOX:
			case APP_TSU_DRAWABLE_TEXTBOX:
			case APP_TSU_DRAWABLE_TRIANGLE:
			case APP_TSU_DRAWABLE_CARDSTACK:
				TSU_DrawableSubRemove((Drawable *)TSU_AppGetScene(app->tsunami), (Drawable *)object);
				break;
			default:
				break;
		}
	}

	switch(type) {
		case APP_TSU_DRAWABLE_LABEL: {
			Label *label = (Label *)object;
			TSU_LabelDestroy(&label);
			break;
		}
		case APP_TSU_DRAWABLE_BANNER: {
			Banner *banner = (Banner *)object;
			TSU_BannerDestroy(&banner);
			break;
		}
		case APP_TSU_DRAWABLE_RECTANGLE: {
			Rectangle *rect = (Rectangle *)object;
			TSU_RectangleDestroy(&rect);
			break;
		}
		case APP_TSU_DRAWABLE_GRADIENT: {
			Gradient *gradient = (Gradient *)object;
			TSU_GradientDestroy(&gradient);
			break;
		}
		case APP_TSU_DRAWABLE_CHECKBOX: {
			CheckBox *checkbox = (CheckBox *)object;
			TSU_CheckBoxDestroy(&checkbox);
			break;
		}
		case APP_TSU_DRAWABLE_TEXTBOX: {
			TextBox *textbox = (TextBox *)object;
			TSU_TextBoxDestroy(&textbox);
			break;
		}
		case APP_TSU_DRAWABLE_CIRCLE: {
			Circle *circle = (Circle *)object;
			TSU_CircleDestroy(&circle);
			break;
		}
		case APP_TSU_DRAWABLE_WAVE: {
			Wave *wave = (Wave *)object;
			TSU_WaveDestroy(&wave);
			break;
		}
		case APP_TSU_DRAWABLE_BOX: {
			Box *box = (Box *)object;
			TSU_BoxDestroy(&box);
			break;
		}
		case APP_TSU_DRAWABLE_TRIANGLE: {
			Triangle *triangle = (Triangle *)object;
			TSU_TriangleDestroy(&triangle);
			break;
		}
		case APP_TSU_DRAWABLE_CARDSTACK: {
			CardStack *stack = (CardStack *)object;
			TSU_CardStackDestroy(&stack);
			break;
		}
		default:
			break;
	}
}

void appTsunamiDestroyApp(App_t *app) {

	DSApp *dsapp;

	if(app == NULL || app->tsunami == NULL) {
		return;
	}

	GUI_CloseApp(app);

	dsapp = app->tsunami;
	TSU_AppDestroy(&dsapp);
	app->tsunami = NULL;
}

void appTsunamiDestroyImage(void *data) {

	Texture *texture = (Texture *)data;

	if(texture != NULL) {
		TSU_TextureDestroy(&texture);
	}
}

void appTsunamiDestroyFont(void *data) {

	Font *font = (Font *)data;

	if(font != NULL) {
		TSU_FontDestroy(&font);
	}
}
