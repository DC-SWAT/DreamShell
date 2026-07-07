#include "drawables/dialog.h"
#include "tsudefinition.h"

static void layoutButtonBg(Rectangle *bg, Label *label, float cx, float cy, float pad_x, float pad_y, float z) {
	float tw;
	float th;
	Vector pos;

	if(bg == nullptr || label == nullptr) {
		return;
	}

	label->getFont()->getTextSize(label->getText(), &tw, &th);

	TSU_RectangleSetSize(bg, tw + pad_x * 2.0f, th + pad_y * 2.0f);
	pos.x = cx - (tw + pad_x * 2.0f) * 0.5f;
	pos.y = cy + (th + pad_y * 2.0f) * 0.5f;
	pos.z = z;
	pos.w = 1.0f;
	TSU_DrawableSetTranslate((Drawable *)bg, &pos);
}

Dialog::Dialog(Font *display_font, float screen_w, float screen_h, const Color &overlay_color, const Color &text_color, const Color &btn_color)
	: Dialog(display_font, screen_w, screen_h, screen_w, screen_h, overlay_color, overlay_color, text_color, btn_color, "Yes", "No", 14) {
}

Dialog::Dialog(Font *display_font, float screen_w, float screen_h, float box_w, float box_h, const Color &overlay_color, const Color &panel_color, const Color &text_color, const Color &btn_color, const char *confirm_text, const char *cancel_text, uint text_size) {
	Color border_color = {1.0f, 1.0f, 1.0f, 0.35f};
	Color focus_bg_color = {0.85f, 1.0f, 1.0f, 0.22f};
	Color idle_bg_color = {1.0f, 1.0f, 1.0f, 0.06f};
	float confirm_cx;
	float cancel_cx;
	float msg_y;
	float btn_y;
	Vector pos;

	setObjectType(ObjectTypeEnum::DIALOG_TYPE);
	m_screen_w = screen_w;
	m_screen_h = screen_h;
	m_box_w = box_w;
	m_box_h = box_h;
	m_text_size = text_size;
	m_visible = false;
	m_focus_btn = 0;
	m_bg_z = 200.0f;
	m_msg_z = 201.5f;
	m_text_color = text_color;
	m_btn_color = btn_color;
	m_btn_dim_color = Color(btn_color.a * 0.45f, btn_color.r * 0.55f, btn_color.g * 0.55f, btn_color.b * 0.55f);
	m_btn_focus_color = Color(1.0f, btn_color.r, btn_color.g, btn_color.b);
	m_confirm_text = confirm_text ? confirm_text : "Yes";
	m_cancel_text = cancel_text ? cancel_text : "No";

	if(m_box_w <= 0.0f) {
		m_box_w = m_screen_w;
	}

	if(m_box_h <= 0.0f) {
		m_box_h = m_screen_h;
	}

	m_panel_x = (m_screen_w - m_box_w) * 0.5f;
	m_panel_y = (m_screen_h - m_box_h) * 0.5f + m_box_h;
	msg_y = m_panel_y - m_box_h * 0.62f;
	btn_y = m_panel_y - m_box_h * 0.24f;
	confirm_cx = m_panel_x + m_box_w * 0.32f;
	cancel_cx = m_panel_x + m_box_w * 0.68f;

	m_overlay = new Rectangle(PVR_LIST_TR_POLY, 0.0f, m_screen_h, m_screen_w, m_screen_h, overlay_color, m_bg_z, 0.0f, border_color, 0.0f);
	m_panel = new Rectangle(PVR_LIST_TR_POLY, m_panel_x, m_panel_y, m_box_w, m_box_h, panel_color, m_bg_z + 0.05f, 2.0f, border_color, 10.0f);
	m_confirm_bg = new Rectangle(PVR_LIST_TR_POLY, 0.0f, 0.0f, 64.0f, 24.0f, focus_bg_color, m_msg_z - 0.2f, 2.0f, border_color, 8.0f);
	m_cancel_bg = new Rectangle(PVR_LIST_TR_POLY, 0.0f, 0.0f, 64.0f, 24.0f, idle_bg_color, m_msg_z - 0.2f, 2.0f, border_color, 8.0f);
	m_msg = new Label(display_font, "", text_size, true, false, false);
	m_confirm = new Label(display_font, m_confirm_text, text_size, true, false, false);
	m_cancel = new Label(display_font, m_cancel_text, text_size, true, false, false);

	m_msg->setTint(text_color);
	m_confirm->setTint(text_color);
	m_cancel->setTint(text_color);

	m_msg->setClickEvent(nullptr);
	m_confirm->setClickEvent(nullptr);
	m_cancel->setClickEvent(nullptr);

	subAdd(m_overlay);
	subAdd(m_panel);
	subAdd(m_confirm_bg);
	subAdd(m_cancel_bg);
	subAdd(m_msg);
	subAdd(m_confirm);
	subAdd(m_cancel);

	pos.x = m_screen_w * 0.5f;
	pos.y = msg_y;
	pos.z = m_msg_z;
	pos.w = 1.0f;
	m_msg->setTranslate(pos);

	pos.x = confirm_cx;
	pos.y = btn_y;
	m_confirm->setTranslate(pos);

	pos.x = cancel_cx;
	m_cancel->setTranslate(pos);

	updateLayout();
	setVisible(false);
}

Dialog::~Dialog() {
	if (m_msg != nullptr) {
		m_msg->setFinished();
	}

	if (m_confirm != nullptr) {
		m_confirm->setFinished();
	}

	if (m_cancel != nullptr) {
		m_cancel->setFinished();
	}

	if (m_confirm_bg != nullptr) {
		m_confirm_bg->setFinished();
	}

	if (m_cancel_bg != nullptr) {
		m_cancel_bg->setFinished();
	}

	if (m_panel != nullptr) {
		m_panel->setFinished();
	}

	if (m_overlay != nullptr) {
		m_overlay->setFinished();
	}

	subRemoveFinished();

	if (m_msg != nullptr) {
		delete m_msg;
		m_msg = nullptr;
	}

	if (m_confirm != nullptr) {
		delete m_confirm;
		m_confirm = nullptr;
	}

	if (m_cancel != nullptr) {
		delete m_cancel;
		m_cancel = nullptr;
	}

	if (m_confirm_bg != nullptr) {
		delete m_confirm_bg;
		m_confirm_bg = nullptr;
	}

	if (m_cancel_bg != nullptr) {
		delete m_cancel_bg;
		m_cancel_bg = nullptr;
	}

	if (m_panel != nullptr) {
		delete m_panel;
		m_panel = nullptr;
	}

	if (m_overlay != nullptr) {
		delete m_overlay;
		m_overlay = nullptr;
	}
}

void Dialog::updateLayout() {
	Vector pos;
	float confirm_cx;
	float cancel_cx;
	float msg_y;
	float btn_y;
	const float btn_pad_x = 14.0f;
	const float btn_pad_y = 8.0f;

	m_panel_x = (m_screen_w - m_box_w) * 0.5f;
	m_panel_y = (m_screen_h - m_box_h) * 0.5f + m_box_h;
	msg_y = m_panel_y - m_box_h * 0.62f;
	btn_y = m_panel_y - m_box_h * 0.24f;
	confirm_cx = m_panel_x + m_box_w * 0.32f;
	cancel_cx = m_panel_x + m_box_w * 0.68f;

	pos.x = 0.0f;
	pos.y = m_screen_h;
	pos.z = m_bg_z;
	pos.w = 1.0f;
	TSU_DrawableSetTranslate((Drawable *)m_overlay, &pos);
	TSU_RectangleSetSize(m_overlay, m_screen_w, m_screen_h);

	pos.x = m_panel_x;
	pos.y = m_panel_y;
	pos.z = m_bg_z + 0.05f;
	TSU_DrawableSetTranslate((Drawable *)m_panel, &pos);
	TSU_RectangleSetSize(m_panel, m_box_w, m_box_h);

	pos.x = m_screen_w * 0.5f;
	pos.y = msg_y;
	pos.z = m_msg_z;
	m_msg->setTranslate(pos);

	pos.x = confirm_cx;
	pos.y = btn_y;
	pos.z = m_msg_z;
	m_confirm->setTranslate(pos);

	pos.x = cancel_cx;
	pos.y = btn_y;
	m_cancel->setTranslate(pos);

	layoutButtonBg(m_confirm_bg, m_confirm, confirm_cx, btn_y, btn_pad_x, btn_pad_y, m_msg_z - 0.2f);
	layoutButtonBg(m_cancel_bg, m_cancel, cancel_cx, btn_y, btn_pad_x, btn_pad_y, m_msg_z - 0.2f);
	updateButtonFocus();
}

void Dialog::updateButtonFocus() {
	Color focus_bg_color = Color(0.95f, m_btn_color.r, m_btn_color.g, m_btn_color.b);
	Color idle_bg_color = {0.0f, 0.0f, 0.0f, 0.0f};
	Color focus_border_color = {1.0f, 1.0f, 1.0f, 1.0f};
	Color idle_border_color = {0.0f, 0.0f, 0.0f, 0.0f};
	Color focus_text_color = {1.0f, 0.10f, 0.10f, 0.10f};
	Color idle_text_color = {0.38f, 0.62f, 0.62f, 0.62f};

	if(m_focus_btn == 0) {
		m_confirm->setTint(focus_text_color);
		m_cancel->setTint(idle_text_color);
		m_confirm_bg->setTint(focus_bg_color);
		m_cancel_bg->setTint(idle_bg_color);
		m_confirm_bg->setBorderColor(focus_border_color);
		m_cancel_bg->setBorderColor(idle_border_color);
	}
	else {
		m_confirm->setTint(idle_text_color);
		m_cancel->setTint(focus_text_color);
		m_confirm_bg->setTint(idle_bg_color);
		m_cancel_bg->setTint(focus_bg_color);
		m_confirm_bg->setBorderColor(idle_border_color);
		m_cancel_bg->setBorderColor(focus_border_color);
	}
}

void Dialog::setVisible(bool visible) {
	Color transparent_border = {0.0f, 0.0f, 0.0f, 0.0f};
	Color panel_border = {1.0f, 1.0f, 1.0f, 0.35f};

	m_visible = visible;
	setAlpha(visible ? 1.0f : 0.0f);
	m_overlay->setAlpha(visible ? 1.0f : 0.0f);
	m_panel->setAlpha(visible ? 1.0f : 0.0f);
	m_msg->setAlpha(visible ? 1.0f : 0.0f);
	m_confirm->setAlpha(visible ? 1.0f : 0.0f);
	m_cancel->setAlpha(visible ? 1.0f : 0.0f);
	m_confirm_bg->setAlpha(visible ? 1.0f : 0.0f);
	m_cancel_bg->setAlpha(visible ? 1.0f : 0.0f);

	if(visible) {
		m_panel->setBorderColor(panel_border);
		updateButtonFocus();
	}
	else {
		m_panel->setBorderColor(transparent_border);
		m_confirm_bg->setBorderColor(transparent_border);
		m_cancel_bg->setBorderColor(transparent_border);
	}
}

void Dialog::draw(pvr_list_type_t list) {
	if(!m_visible) {
		return;
	}

	Drawable::draw(list);
}

void Dialog::show(const char *text) {
	if (text != nullptr) {
		m_msg->setText(text);
	}

	m_focus_btn = 0;
	updateLayout();
	setVisible(true);
}

void Dialog::hide() {
	setVisible(false);
}

bool Dialog::isVisible() const {
	return m_visible;
}

void Dialog::setConfirmText(const char *text) {
	if (text == nullptr) {
		return;
	}

	m_confirm_text = text;
	m_confirm->setText(m_confirm_text);
	updateLayout();
}

void Dialog::setCancelText(const char *text) {
	if (text == nullptr) {
		return;
	}

	m_cancel_text = text;
	m_cancel->setText(m_cancel_text);
	updateLayout();
}

void Dialog::setConfirmCallback(ClickEventFunctionPtr callback, int callback_id) {
	if (m_confirm != nullptr) {
		m_confirm->setClickEvent(callback);

		if (callback_id > 0) {
			m_confirm->setId(callback_id);
		}
	}
}

void Dialog::setCancelCallback(ClickEventFunctionPtr callback, int callback_id) {
	if (m_cancel != nullptr) {
		m_cancel->setClickEvent(callback);

		if (callback_id > 0) {
			m_cancel->setId(callback_id);
		}
	}
}

void Dialog::setFocus(int button) {
	if(button < 0) {
		button = 0;
	}

	if(button > 1) {
		button = 1;
	}

	m_focus_btn = button;
	updateButtonFocus();
}

void Dialog::moveFocus(int dir) {
	if(dir < 0) {
		m_focus_btn = 0;
	}
	else if(dir > 0) {
		m_focus_btn = 1;
	}

	updateButtonFocus();
}

void Dialog::activateFocused() {
	if (!m_visible) {
		return;
	}

	if(m_focus_btn == 0) {
		m_confirm->click();
	}
	else {
		m_cancel->click();
	}
}

void Dialog::handleClick(int mx, int my) {
	if (!m_visible) {
		return;
	}

	if (TSU_DrawableIsMouseInside((Drawable *)m_confirm, mx, my) ||
			TSU_DrawableIsMouseInside((Drawable *)m_confirm_bg, mx, my)) {
		m_focus_btn = 0;
		updateButtonFocus();
		m_confirm->click();
	}
	else if (TSU_DrawableIsMouseInside((Drawable *)m_cancel, mx, my) ||
			TSU_DrawableIsMouseInside((Drawable *)m_cancel_bg, mx, my)) {
		m_focus_btn = 1;
		updateButtonFocus();
		m_cancel->click();
	}
}

extern "C"
{
	Dialog* TSU_DialogCreate(Font *display_font, float screen_w, float screen_h, Color *bg_color, Color *text_color, Color *btn_color)
	{
		if (display_font == nullptr || bg_color == nullptr || text_color == nullptr || btn_color == nullptr) {
			return nullptr;
		}

		return new Dialog(display_font, screen_w, screen_h, *bg_color, *text_color, *btn_color);
	}

	Dialog* TSU_DialogCreateEx(Font *display_font, float screen_w, float screen_h, float box_w, float box_h, Color *overlay_color, Color *panel_color, Color *text_color, Color *btn_color, const char *confirm_text, const char *cancel_text, uint text_size)
	{
		Color default_panel = {0.95f, 0.16f, 0.16f, 0.35f};

		if (display_font == nullptr || overlay_color == nullptr || text_color == nullptr || btn_color == nullptr) {
			return nullptr;
		}

		if(panel_color == nullptr) {
			panel_color = &default_panel;
		}

		return new Dialog(display_font, screen_w, screen_h, box_w, box_h, *overlay_color, *panel_color, *text_color, *btn_color, confirm_text, cancel_text, text_size);
	}

	void TSU_DialogDestroy(Dialog **dialog_ptr)
	{
		if (dialog_ptr != nullptr && *dialog_ptr != nullptr) {
			delete *dialog_ptr;
			*dialog_ptr = nullptr;
		}
	}

	void TSU_DialogSetConfirmText(Dialog *dialog_ptr, const char *text)
	{
		if (dialog_ptr != nullptr) {
			dialog_ptr->setConfirmText(text);
		}
	}

	void TSU_DialogSetCancelText(Dialog *dialog_ptr, const char *text)
	{
		if (dialog_ptr != nullptr) {
			dialog_ptr->setCancelText(text);
		}
	}

	void TSU_DialogSetConfirmCallback(Dialog *dialog_ptr, ClickEventFunctionPtr callback, int callback_id)
	{
		if (dialog_ptr != nullptr) {
			dialog_ptr->setConfirmCallback(callback, callback_id);
		}
	}

	void TSU_DialogSetCancelCallback(Dialog *dialog_ptr, ClickEventFunctionPtr callback, int callback_id)
	{
		if (dialog_ptr != nullptr) {
			dialog_ptr->setCancelCallback(callback, callback_id);
		}
	}

	void TSU_DialogShow(Dialog *dialog_ptr, const char *text)
	{
		if (dialog_ptr != nullptr) {
			dialog_ptr->show(text);
		}
	}

	void TSU_DialogHide(Dialog *dialog_ptr)
	{
		if (dialog_ptr != nullptr) {
			dialog_ptr->hide();
		}
	}

	int TSU_DialogIsVisible(Dialog *dialog_ptr)
	{
		if (dialog_ptr != nullptr) {
			return dialog_ptr->isVisible() ? 1 : 0;
		}

		return 0;
	}

	void TSU_DialogSetFocus(Dialog *dialog_ptr, int button)
	{
		if (dialog_ptr != nullptr) {
			dialog_ptr->setFocus(button);
		}
	}

	void TSU_DialogMoveFocus(Dialog *dialog_ptr, int dir)
	{
		if (dialog_ptr != nullptr) {
			dialog_ptr->moveFocus(dir);
		}
	}

	void TSU_DialogActivateFocused(Dialog *dialog_ptr)
	{
		if (dialog_ptr != nullptr) {
			dialog_ptr->activateFocused();
		}
	}

	void TSU_DialogHandleClick(Dialog *dialog_ptr, int mx, int my)
	{
		if (dialog_ptr != nullptr) {
			dialog_ptr->handleClick(mx, my);
		}
	}
}
