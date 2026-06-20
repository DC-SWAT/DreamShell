/*
   Tsunami for KallistiOS ##version##

   cardstack.h

   Copyright (C) 2026 SWAT

*/

#ifndef __TSUNAMI_DRW_CARDSTACK_H
#define __TSUNAMI_DRW_CARDSTACK_H

#include "../drawable.h"
#include "../color.h"
#include "scene.h"
#include "rectangle.h"
#include "banner.h"

#ifdef __cplusplus

#include <deque>
#include <string>

enum CardStackTransition {
	CARDSTACK_TRANSITION_NONE = 0,
	CARDSTACK_TRANSITION_OUT,
	CARDSTACK_TRANSITION_IN
};

class CardStack : public Drawable {
public:
	CardStack(float width, float height);
	virtual ~CardStack();

	void addCard(Scene *scene, const char *name);
	void setBackgroundColor(const Color &color, pvr_list_type_t list, float z);
	void setBackground(Banner *banner);
	void setFadeStep(float step);

	void nextCard();
	void prevCard();
	void showIndex(int index);
	void setIndex(int index);
	void show(const char *name);
	int getIndex() const;
	int isTransitioning() const;

	virtual void draw(pvr_list_type_t list);
	virtual void nextFrame();

private:
	static const float ALPHA_MIN;
	static const float ALPHA_MAX;
	static const float DEFAULT_FADE_STEP;

	int m_visible_index;
	int m_target_index;
	int m_transition;
	float m_fade_alpha;
	float m_fade_step;
	Rectangle *m_background_rect;
	Banner *m_background_banner;
	std::deque<Scene *> m_cards;
	std::deque<std::string> m_card_names;

	void clampVisibleIndex();
	void setCardAlpha(int index, float alpha);
	void finishTransitionTo(int index);
	void beginTransition(int index);
	void advanceTransition();
};

#else

typedef struct cardstack CardStack;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

CardStack *TSU_CardStackCreate(float width, float height);
void TSU_CardStackDestroy(CardStack **cardstack_ptr);
int TSU_CardStackCheck(Drawable *drawable_ptr);
void TSU_CardStackAddCard(CardStack *cardstack_ptr, Scene *scene_ptr, const char *name);
void TSU_CardStackSetBackgroundColor(CardStack *cardstack_ptr, const Color *color, pvr_list_type_t list, float z);
void TSU_CardStackSetBackground(CardStack *cardstack_ptr, Banner *banner_ptr);
void TSU_CardStackSetFadeStep(CardStack *cardstack_ptr, float step);
void TSU_CardStackNext(CardStack *cardstack_ptr);
void TSU_CardStackPrev(CardStack *cardstack_ptr);
void TSU_CardStackShowIndex(CardStack *cardstack_ptr, int index);
void TSU_CardStackSetIndex(CardStack *cardstack_ptr, int index);
void TSU_CardStackShow(CardStack *cardstack_ptr, const char *name);
int TSU_CardStackGetIndex(CardStack *cardstack_ptr);
int TSU_CardStackIsTransitioning(CardStack *cardstack_ptr);

#ifdef __cplusplus
};
#endif

#endif /* __TSUNAMI_DRW_CARDSTACK_H */
