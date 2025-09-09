#include <string.h>
#include <stdlib.h>
#include "SDL_gui.h"

// Implementation of GUI_Dialog
GUI_Dialog::GUI_Dialog(const char *aname, int x, int y, int w, int h, GUI_Font *font)
: GUI_Container(aname, x, y, w, h) {
    GUI_Surface *bg_surface = CreateBackground(w, h);
    SDL_PixelFormat *format = GUI_GetScreen()->GetSurface()->GetSurface()->format;
    SDL_Rect rect;

    SetTransparent(1);

    GUI_Panel *content_panel = new GUI_Panel("dialog_content", 0, 0, w, h);
    content_panel->SetBackgroundCenter(bg_surface);
    AddWidget(content_panel);
    content_panel->DecRef();
    bg_surface->DecRef();

    // Label
    label = new GUI_Label("dialog_label", 10, 10, w - 20, 40, font, "Message");
    label->SetTextColor(0, 0, 0);
    content_panel->AddWidget(label);
    label->DecRef();

    // RTF
    rtf = new GUI_RTF("dialog_rtf", "Message", 0, 0, w - 20, h - 100, (const char *)NULL);
    rtf->SetBgColor(217, 217, 217);

    // Body panel
    body = new GUI_Panel("dialog_body", 10, 45, w - 20, h - 100);
    body->SetTransparent(1);
    content_panel->AddWidget(body);
    body->DecRef();

    // Input
    GUI_Surface *input_normal = new GUI_Surface("input_normal", SDL_HWSURFACE, w - 40, 30,
        format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
    rect = {0, 0, (Uint16)(w - 40), 30};
    input_normal->Fill(&rect, SDL_MapRGB(format, 238, 238, 238));
    rect = {1, 1, (Uint16)(w - 42), 28};
    input_normal->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
    rect = {2, 2, (Uint16)(w - 44), 26};
    input_normal->Fill(&rect, SDL_MapRGB(format, 245, 245, 245));

    GUI_Surface *input_focus = new GUI_Surface("input_focus", SDL_HWSURFACE, w - 40, 30,
        format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
    rect = {0, 0, (Uint16)(w - 40), 30};
    input_focus->Fill(&rect, SDL_MapRGB(format, 238, 238, 238));
    rect = {1, 1, (Uint16)(w - 42), 28};
    input_focus->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
    rect = {2, 2, (Uint16)(w - 44), 26};
    input_focus->Fill(&rect, SDL_MapRGB(format, 217, 245, 255));

    GUI_Surface *input_highlight = new GUI_Surface("input_highlight", SDL_HWSURFACE, w - 40, 30,
        format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
    rect = {0, 0, (Uint16)(w - 40), 30};
    input_highlight->Fill(&rect, SDL_MapRGB(format, 238, 238, 238));
    rect = {1, 1, (Uint16)(w - 42), 28};
    input_highlight->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
    rect = {2, 2, (Uint16)(w - 44), 26};
    input_highlight->Fill(&rect, SDL_MapRGB(format, 255, 255, 224));

    input = new GUI_TextEntry("dialog_input", 10, (Sint16)((body->GetHeight() - 30) / 2),
        (Uint16)(w - 40), 30, font, 256);
    input->SetNormalImage(input_normal);
    input->SetHighlightImage(input_highlight);
    input->SetFocusImage(input_focus);
    input->SetTextColor(0, 0, 0);
    input_normal->DecRef();
    input_focus->DecRef();
    input_highlight->DecRef();

    // Progress bar
    GUI_Surface *progress_bg = new GUI_Surface("progress_bg", SDL_HWSURFACE, w - 40, 20,
        format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
    rect = {0, 0, (Uint16)(w-40), 20};
    progress_bg->Fill(&rect, SDL_MapRGB(format, 238, 238, 238));
    rect = {1, 1, (Uint16)(w-42), 18};
    progress_bg->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
    rect = {2, 2, (Uint16)(w-44), 16};
    progress_bg->Fill(&rect, SDL_MapRGB(format, 217, 217, 217));

    GUI_Surface *progress_bar_surf = new GUI_Surface("progress_bar_surf", SDL_HWSURFACE, w - 40, 20,
        format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
    rect = {0, 0, (Uint16)(w-40), 20};
    progress_bar_surf->Fill(&rect, SDL_MapRGB(format, 238, 238, 238));
    rect = {1, 1, (Uint16)(w-42), 18};
    progress_bar_surf->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
    rect = {2, 2, (Uint16)(w-44), 16};
    progress_bar_surf->Fill(&rect, SDL_MapRGB(format, 49, 121, 159));
    
    progress = new GUI_ProgressBar("dialog_progress", 10, (Sint16)((body->GetHeight() - 20) / 2),
        (Uint16)(w - 40), 20);
    progress->SetImage1(progress_bg);
    progress->SetImage2(progress_bar_surf);
    progress_bg->DecRef();
    progress_bar_surf->DecRef();

    // Buttons panel
    GUI_Panel *buttons_panel = new GUI_Panel("dialog_buttons_panel", 10, h - 50, w - 20, 40);
    buttons_panel->SetTransparent(1);
    content_panel->AddWidget(buttons_panel);
    buttons_panel->DecRef();

    // Button surfaces
    GUI_Surface *btn_normal = new GUI_Surface("btn_normal", SDL_HWSURFACE, 100, 30,
        format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
    rect = {0, 0, 100, 30};
    btn_normal->Fill(&rect, SDL_MapRGB(format, 238, 238, 238));
    rect = {1, 1, 98, 28};
    btn_normal->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
    rect = {2, 2, 96, 26};
    btn_normal->Fill(&rect, SDL_MapRGB(format, 49, 121, 159));

    GUI_Surface *btn_highlight = new GUI_Surface("btn_highlight", SDL_HWSURFACE, 100, 30,
        format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
    rect = {0, 0, 100, 30};
    btn_highlight->Fill(&rect, SDL_MapRGB(format, 238, 238, 238));
    rect = {1, 1, 98, 28};
    btn_highlight->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
    rect = {2, 2, 96, 26};
    btn_highlight->Fill(&rect, SDL_MapRGB(format, 97, 189, 236));

    GUI_Surface *btn_pressed = new GUI_Surface("btn_pressed", SDL_HWSURFACE, 100, 30,
        format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
    rect = {0, 0, 100, 30};
    btn_pressed->Fill(&rect, SDL_MapRGB(format, 238, 238, 238));
    rect = {1, 1, 98, 28};
    btn_pressed->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
    rect = {2, 2, 96, 26};
    btn_pressed->Fill(&rect, SDL_MapRGB(format, 212, 241, 41));

    GUI_Surface *btn_disabled = new GUI_Surface("btn_disabled", SDL_HWSURFACE, 100, 30,
        format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
    rect = {0, 0, 100, 30};
    btn_disabled->Fill(&rect, SDL_MapRGB(format, 238, 238, 238));
    rect = {1, 1, 98, 28};
    btn_disabled->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
    rect = {2, 2, 96, 26};
    btn_disabled->Fill(&rect, SDL_MapRGB(format, 204, 204, 204));

    // Confirm button
    confirm_button = new GUI_Button("dialog_confirm", (Sint16)((w-20)/2 - 110), 0, 100, 30);
    confirm_button->SetNormalImage(btn_normal);
    confirm_button->SetHighlightImage(btn_highlight);
    confirm_button->SetPressedImage(btn_pressed);
    confirm_button->SetDisabledImage(btn_disabled);

    GUI_Label *confirm_label = new GUI_Label("confirm_label", 0, 0, 100, 30, font, "Confirm");
    confirm_label->SetTextColor(255,255,255);
    confirm_button->SetCaption(confirm_label);
    confirm_button->SetEnabled(1);
    confirm_label->DecRef();

    GUI_EventHandler<GUI_Dialog> *confirm_cb_handler = new GUI_EventHandler<GUI_Dialog>(this, &GUI_Dialog::ButtonClick);
    confirm_button->SetClick(confirm_cb_handler);
    confirm_cb_handler->DecRef();
    buttons_panel->AddWidget(confirm_button);
    confirm_button->DecRef();

    // Cancel button
    cancel_button = new GUI_Button("dialog_cancel", (Sint16)((w-20)/2 + 10), 0, 100, 30);
    cancel_button->SetNormalImage(btn_normal);
    cancel_button->SetHighlightImage(btn_highlight);
    cancel_button->SetPressedImage(btn_pressed);
    cancel_button->SetDisabledImage(btn_disabled);

    GUI_Label *cancel_label = new GUI_Label("cancel_label", 0, 0, 100, 30, font, "Cancel");
    cancel_label->SetTextColor(255,255,255);
    cancel_button->SetCaption(cancel_label);
    cancel_button->SetEnabled(1);
    cancel_label->DecRef();

    GUI_EventHandler<GUI_Dialog> *cancel_cb_handler = new GUI_EventHandler<GUI_Dialog>(this, &GUI_Dialog::ButtonClick);
    cancel_button->SetClick(cancel_cb_handler);
    cancel_cb_handler->DecRef();
    buttons_panel->AddWidget(cancel_button);
    cancel_button->DecRef();

    btn_normal->DecRef();
    btn_highlight->DecRef();
    btn_pressed->DecRef();
    btn_disabled->DecRef();

    confirm_callback = NULL;
    cancel_callback = NULL;

    original_h = GetHeight();
    original_y = GetArea().y;

    SetFlags(WIDGET_HIDDEN);
}

GUI_Dialog::~GUI_Dialog(void) {
    rtf->DecRef();
    input->DecRef();
    progress->DecRef();
    if (confirm_callback) confirm_callback->DecRef();
    if (cancel_callback) cancel_callback->DecRef();
}

GUI_Surface *GUI_Dialog::CreateBackground(int w, int h) {
    SDL_PixelFormat *format = GUI_GetScreen()->GetSurface()->GetSurface()->format;
    SDL_Rect rect;

    GUI_Surface *bg_surface = new GUI_Surface("dialog_bg", SDL_HWSURFACE, w, h,
        format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);

    rect = {0, 0, (Uint16)w, (Uint16)h};
    bg_surface->Fill(&rect, SDL_MapRGB(format, 85, 85, 85));
    rect = {0, 0, (Uint16)w, (Uint16)(h - 1)};
    bg_surface->Fill(&rect, SDL_MapRGB(format, 170, 170, 170));
    rect = {1, 1, (Uint16)(w - 2), (Uint16)(h - 3)};
    bg_surface->Fill(&rect, SDL_MapRGB(format, 238, 238, 238));
    rect = {2, 2, (Uint16)(w - 4), (Uint16)(h - 5)};
    bg_surface->Fill(&rect, SDL_MapRGB(format, 217, 217, 217));
    rect = {2, 2, (Uint16)(w - 4), 1};
    bg_surface->Fill(&rect, SDL_MapRGB(format, 251, 251, 251));

    return bg_surface;
}

void GUI_Dialog::RelayoutButtons(void) {
    GUI_Panel *buttons_panel = (GUI_Panel *)confirm_button->GetParent();
    Sint16 panel_width = buttons_panel->GetWidth();
    Sint16 panel_height = buttons_panel->GetHeight();
    Sint16 panel_center = panel_width / 2;
    Sint16 gap = 20;

    Sint16 confirm_w = confirm_button->GetWidth();
    Sint16 confirm_h = confirm_button->GetHeight();
    Sint16 cancel_h = cancel_button->GetHeight();

    Sint16 confirm_x = panel_center - (gap/2) - confirm_w;
    Sint16 confirm_y = (panel_height - confirm_h) / 2;
    Sint16 cancel_x = panel_center + (gap/2);
    Sint16 cancel_y = (panel_height - cancel_h) / 2;

    confirm_button->SetPosition(confirm_x, confirm_y);
    cancel_button->SetPosition(cancel_x, cancel_y);
}

void GUI_Dialog::ButtonClick(GUI_Object *sender) {
    if(sender == confirm_button) {
        Hide();
        if(confirm_callback) {
            confirm_callback->Call(this);
        }
    }
    else if(sender == cancel_button) {
        Hide();
        if(cancel_callback) {
            cancel_callback->Call(this);
        }
    }
}

void GUI_Dialog::Show(DialogMode new_mode, const char *text, const char *bodyText) {
	
    GUI_Panel *content_panel = (GUI_Panel *)widgets[0];
    GUI_Panel *buttons_panel = (GUI_Panel *)confirm_button->GetParent();

    if (GetHeight() != original_h) {
        SetPosition(area.x, original_y);
        SetHeight(original_h);
        content_panel->SetHeight(original_h);
        body->SetHeight(original_h - 100);
        rtf->SetHeight(original_h - 100);
        buttons_panel->SetPosition(buttons_panel->GetArea().x, original_h - 50);

        GUI_Surface *new_bg = CreateBackground(content_panel->GetWidth(), content_panel->GetHeight());
        content_panel->SetBackgroundCenter(new_bg);
        new_bg->DecRef();
    }
	
    mode = new_mode;
    label->SetText(text);
    body->RemoveAllWidgets();
    bool rtf_is_used = false;

    switch(mode) {
        case MODE_INFO:
            if (bodyText) {
                rtf->SetText(bodyText);
                body->AddWidget(rtf);
                rtf_is_used = true;
            }
            confirm_button->SetEnabled(0);
            ((GUI_Label *)confirm_button->GetCaption())->SetTextColor(0, 0, 0);
            cancel_button->SetEnabled(0);
            ((GUI_Label *)cancel_button->GetCaption())->SetTextColor(0, 0, 0);
            break;
        case MODE_ALERT:
            if (bodyText) {
                rtf->SetText(bodyText);
                body->AddWidget(rtf);
                rtf_is_used = true;
            }
            confirm_button->SetEnabled(1);
            ((GUI_Label *)confirm_button->GetCaption())->SetTextColor(255, 255, 255);
            cancel_button->SetEnabled(0);
            ((GUI_Label *)cancel_button->GetCaption())->SetTextColor(0, 0, 0);
            break;
        case MODE_CONFIRM:
            if (bodyText) {
                rtf->SetText(bodyText);
                body->AddWidget(rtf);
                rtf_is_used = true;
            }
            confirm_button->SetEnabled(1);
            ((GUI_Label *)confirm_button->GetCaption())->SetTextColor(255, 255, 255);
            cancel_button->SetEnabled(1);
            ((GUI_Label *)cancel_button->GetCaption())->SetTextColor(255, 255, 255);
            break;
        case MODE_PROMPT:
            input->SetText(bodyText ? bodyText : "");
            body->AddWidget(input);
            confirm_button->SetEnabled(1);
            ((GUI_Label *)confirm_button->GetCaption())->SetTextColor(255, 255, 255);
            cancel_button->SetEnabled(1);
            ((GUI_Label *)cancel_button->GetCaption())->SetTextColor(255, 255, 255);
            break;
        case MODE_PROGRESS:
            body->AddWidget(progress);
            progress->SetPosition(0);
            confirm_button->SetEnabled(0);
            ((GUI_Label *)confirm_button->GetCaption())->SetTextColor(0, 0, 0);
            cancel_button->SetEnabled(1);
            ((GUI_Label *)cancel_button->GetCaption())->SetTextColor(255, 255, 255);
            break;
    }

    if (rtf_is_used) {
        int needed_height = rtf->GetFullHeight();
        int current_height = rtf->GetHeight();

        if (needed_height > current_height) {
            int delta_h = needed_height - current_height;
            GUI_Panel *content_panel = (GUI_Panel *)widgets[0];
            GUI_Panel *buttons_panel = (GUI_Panel *)confirm_button->GetParent();

            SetHeight(GetHeight() + delta_h);
            content_panel->SetHeight(content_panel->GetHeight() + delta_h);
            body->SetHeight(body->GetHeight() + delta_h);
            rtf->SetHeight(needed_height);
            buttons_panel->SetPosition(buttons_panel->GetArea().x, GetHeight() - 50);

            GUI_Screen *screen = GUI_GetScreen();
            if (screen) {
                SetPosition(area.x, (screen->GetHeight() - GetHeight()) / 2);
            }

            GUI_Surface *new_bg = CreateBackground(content_panel->GetWidth(), content_panel->GetHeight());
            content_panel->SetBackgroundCenter(new_bg);
            new_bg->DecRef();
        }
    }

    GUI_Screen *screen = GUI_GetScreen();
    if(screen) screen->SetModalWidget(this);

    ClearFlags(WIDGET_HIDDEN);
    MarkChanged();

    if(parent) {
        parent->MarkChanged();
    }
}

int GUI_Dialog::Event(const SDL_Event *event, int xoffset, int yoffset) {
	int rv;

	if((flags & WIDGET_HIDDEN)) {
		return 0;
	}

	rv = GUI_Drawable::Event(event, xoffset, yoffset);

	if((flags & WIDGET_DISABLED)) {
		return rv;
	}

	// Handle ESCAPE key press to cancel
	if(event->type == SDL_KEYDOWN) {
		if(event->key.keysym.sym == SDLK_ESCAPE) {
			if(!(cancel_button->GetFlags() & WIDGET_DISABLED)) {
				ButtonClick(cancel_button);
				return 1; // Event handled
			}
		}
	}

	if(widgets[0]->Event(event, xoffset + area.x, yoffset + area.y)) {
		return 1;
	}

	return rv;
}

void GUI_Dialog::Hide(void) {
    SetFlags(WIDGET_HIDDEN);

    GUI_Screen *screen = GUI_GetScreen();
    if(screen) screen->SetModalWidget(NULL);

    if(parent) {
        parent->MarkChanged();
    }
}

void GUI_Dialog::SetText(const char *text) {
    label->SetText(text);
}

void GUI_Dialog::SetConfirmCallback(GUI_Callback *callback) {
    GUI_ObjectKeep((GUI_Object **) &confirm_callback, callback);
}

void GUI_Dialog::SetCancelCallback(GUI_Callback *callback) {
    GUI_ObjectKeep((GUI_Object **) &cancel_callback, callback);
}

void GUI_Dialog::SetConfirmButtonImages(GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled) {
	if(normal) {
        int w = normal->GetWidth();
        int h = normal->GetHeight();
        confirm_button->SetNormalImage(normal);
        confirm_button->SetSize(w, h);
        GUI_Widget *caption = confirm_button->GetCaption();
        if(caption) {
            caption->SetSize(w, h);
        }
    }
	if(highlight) confirm_button->SetHighlightImage(highlight);
	if(pressed) confirm_button->SetPressedImage(pressed);
	if(disabled) confirm_button->SetDisabledImage(disabled);
	RelayoutButtons();
	MarkChanged();
}

void GUI_Dialog::SetCancelButtonImages(GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled) {
	if(normal) {
        int w = normal->GetWidth();
        int h = normal->GetHeight();
        cancel_button->SetNormalImage(normal);
        cancel_button->SetSize(w, h);
        GUI_Widget *caption = cancel_button->GetCaption();
        if(caption) {
            caption->SetSize(w, h);
        }
    }
	if(highlight) cancel_button->SetHighlightImage(highlight);
	if(pressed) cancel_button->SetPressedImage(pressed);
	if(disabled) cancel_button->SetDisabledImage(disabled);
	RelayoutButtons();
	MarkChanged();
}

void GUI_Dialog::SetInputImages(GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *focus) {
	if(normal) {
        int w = normal->GetWidth();
        int h = normal->GetHeight();
        input->SetNormalImage(normal);
        input->SetSize(w, h);
        input->SetPosition(input->GetArea().x, (body->GetHeight() - h) / 2);
    }
	if(highlight) input->SetHighlightImage(highlight);
	if(focus) input->SetFocusImage(focus);
	MarkChanged();
}

void GUI_Dialog::SetProgressImages(GUI_Surface *bg, GUI_Surface *bar) {
	if(bg) {
        int w = bg->GetWidth();
        int h = bg->GetHeight();
        progress->SetImage1(bg);
        progress->SetSize(w, h);
        ((GUI_Widget *)progress)->SetPosition(progress->GetArea().x, (body->GetHeight() - h) / 2);
    }
	if(bar) progress->SetImage2(bar);
	MarkChanged();
}

const char *GUI_Dialog::GetInputText(void) {
    return input->GetText();
}

void GUI_Dialog::SetProgress(double value) {
    if(mode == MODE_PROGRESS) {
        progress->SetPosition(value);
    }
}

extern "C" {

GUI_Widget *GUI_DialogCreate(const char *name, int x, int y, int w, int h, GUI_Font *font) {
	return new GUI_Dialog(name, x, y, w, h, font);
}

void GUI_DialogShow(GUI_Widget *widget, GUI_DialogMode mode, const char *text, const char *bodyText) {
    ((GUI_Dialog *) widget)->Show((GUI_Dialog::DialogMode)mode, text, bodyText);
}

void GUI_DialogHide(GUI_Widget *widget) {
    ((GUI_Dialog *) widget)->Hide();
}

void GUI_DialogSetText(GUI_Widget *widget, const char *text) {
    ((GUI_Dialog *) widget)->SetText(text);
}

void GUI_DialogSetConfirmCallback(GUI_Widget *widget, GUI_Callback *callback) {
    ((GUI_Dialog *) widget)->SetConfirmCallback(callback);
}

void GUI_DialogSetCancelCallback(GUI_Widget *widget, GUI_Callback *callback) {
    ((GUI_Dialog *) widget)->SetCancelCallback(callback);
}

const char *GUI_DialogGetInputText(GUI_Widget *widget) {
    return ((GUI_Dialog *) widget)->GetInputText();
}

void GUI_DialogSetProgress(GUI_Widget *widget, double value) {
    ((GUI_Dialog *) widget)->SetProgress(value);
}

void GUI_DialogSetConfirmButtonImages(GUI_Widget *widget, GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled) {
	((GUI_Dialog *) widget)->SetConfirmButtonImages(normal, highlight, pressed, disabled);
}

void GUI_DialogSetCancelButtonImages(GUI_Widget *widget, GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled) {
	((GUI_Dialog *) widget)->SetCancelButtonImages(normal, highlight, pressed, disabled);
}

void GUI_DialogSetInputImages(GUI_Widget *widget, GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *focus) {
	((GUI_Dialog *) widget)->SetInputImages(normal, highlight, focus);
}

void GUI_DialogSetProgressImages(GUI_Widget *widget, GUI_Surface *bg, GUI_Surface *bar) {
	((GUI_Dialog *) widget)->SetProgressImages(bg, bar);
}

}
