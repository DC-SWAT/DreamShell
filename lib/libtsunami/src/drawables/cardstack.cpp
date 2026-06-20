/*
   Tsunami for KallistiOS ##version##

   cardstack.cpp

   Copyright (C) 2026 SWAT

*/

#include "drawables/cardstack.h"
#include <deque>
#include <string>

const float CardStack::ALPHA_MIN = 0.0f;
const float CardStack::ALPHA_MAX = 1.0f;
const float CardStack::DEFAULT_FADE_STEP = 0.12f;

CardStack::CardStack(float width, float height)
{
	m_visible_index = 0;
	m_target_index = 0;
	m_transition = CARDSTACK_TRANSITION_NONE;
	m_fade_alpha = ALPHA_MAX;
	m_fade_step = DEFAULT_FADE_STEP;
	m_background_rect = nullptr;
	m_background_banner = nullptr;
	setObjectType(ObjectTypeEnum::CARDSTACK_TYPE);
	setSize(width, height);
}

CardStack::~CardStack()
{
	if (m_background_rect != nullptr) {
		delete m_background_rect;
		m_background_rect = nullptr;
	}

	if (m_background_banner != nullptr) {
		delete m_background_banner;
		m_background_banner = nullptr;
	}

	while (!m_cards.empty()) {
		delete m_cards.front();
		m_cards.pop_front();
	}

	m_card_names.clear();
}

void CardStack::setCardAlpha(int index, float alpha)
{
	Scene *card;

	if (index < 0 || index >= (int)m_cards.size()) {
		return;
	}

	card = m_cards[index];

	if (card != nullptr) {
		card->setAlpha(alpha);
	}
}

void CardStack::finishTransitionTo(int index)
{
	size_t i;

	clampVisibleIndex();

	if (m_cards.empty()) {
		m_transition = CARDSTACK_TRANSITION_NONE;
		return;
	}

	if (index < 0 || index >= (int)m_cards.size()) {
		index = 0;
	}

	for (i = 0; i < m_cards.size(); i++) {
		setCardAlpha((int)i, ALPHA_MIN);
	}

	m_visible_index = index;
	m_target_index = index;
	m_fade_alpha = ALPHA_MAX;
	setCardAlpha(m_visible_index, ALPHA_MAX);
	m_transition = CARDSTACK_TRANSITION_NONE;
}

void CardStack::beginTransition(int index)
{
	Scene *card;

	if (m_cards.empty()) {
		return;
	}

	if (index < 0 || index >= (int)m_cards.size()) {
		return;
	}

	if (m_transition != CARDSTACK_TRANSITION_NONE && index == m_target_index) {
		return;
	}

	if (index == m_visible_index && m_transition == CARDSTACK_TRANSITION_NONE) {
		return;
	}

	m_target_index = index;

	if (m_fade_step <= ALPHA_MIN) {
		finishTransitionTo(index);
		return;
	}

	if (m_transition == CARDSTACK_TRANSITION_IN) {
		finishTransitionTo(m_visible_index);
	}

	if (m_visible_index == m_target_index) {
		m_transition = CARDSTACK_TRANSITION_NONE;
		return;
	}

	m_transition = CARDSTACK_TRANSITION_OUT;
	card = m_cards[m_visible_index];
	m_fade_alpha = card != nullptr ? card->getAlpha() : ALPHA_MAX;

	if (m_fade_alpha <= ALPHA_MIN) {
		m_fade_alpha = ALPHA_MAX;
	}
}

void CardStack::advanceTransition()
{
	if (m_transition == CARDSTACK_TRANSITION_NONE) {
		return;
	}

	if (m_transition == CARDSTACK_TRANSITION_OUT) {
		m_fade_alpha -= m_fade_step;

		if (m_fade_alpha <= ALPHA_MIN) {
			setCardAlpha(m_visible_index, ALPHA_MIN);
			m_visible_index = m_target_index;
			m_transition = CARDSTACK_TRANSITION_IN;
			m_fade_alpha = ALPHA_MIN;
			setCardAlpha(m_visible_index, ALPHA_MIN);
		}
		else {
			setCardAlpha(m_visible_index, m_fade_alpha);
		}
	}
	else {
		m_fade_alpha += m_fade_step;

		if (m_fade_alpha >= ALPHA_MAX) {
			setCardAlpha(m_visible_index, ALPHA_MAX);
			m_transition = CARDSTACK_TRANSITION_NONE;
			m_fade_alpha = ALPHA_MAX;
		}
		else {
			setCardAlpha(m_visible_index, m_fade_alpha);
		}
	}
}

void CardStack::addCard(Scene *scene, const char *name)
{
	int index;

	if (scene == nullptr) {
		return;
	}

	m_cards.push_back(scene);
	index = (int)m_cards.size() - 1;

	if (name != nullptr) {
		m_card_names.push_back(name);
	}
	else {
		m_card_names.push_back("");
	}

	setCardAlpha(index, index == m_visible_index ? ALPHA_MAX : ALPHA_MIN);
}

void CardStack::setBackgroundColor(const Color &color, pvr_list_type_t list, float z)
{
	Vector pos = getTranslate();
	float width = 0.0f;
	float height = 0.0f;

	getSize(&width, &height);

	if (m_background_rect != nullptr) {
		delete m_background_rect;
		m_background_rect = nullptr;
	}

	if (m_background_banner != nullptr) {
		delete m_background_banner;
		m_background_banner = nullptr;
	}

	m_background_rect = new Rectangle(list,
		pos.x,
		pos.y + height,
		width,
		height,
		color,
		z,
		0.0f,
		color,
		0.0f);
}

void CardStack::setBackground(Banner *banner)
{
	if (banner == nullptr) {
		return;
	}

	if (m_background_rect != nullptr) {
		delete m_background_rect;
		m_background_rect = nullptr;
	}

	if (m_background_banner != nullptr) {
		delete m_background_banner;
	}

	m_background_banner = banner;
}

void CardStack::setFadeStep(float step)
{
	if (step < ALPHA_MIN) {
		m_fade_step = ALPHA_MIN;
	}
	else {
		m_fade_step = step;
	}
}

void CardStack::clampVisibleIndex()
{
	if (m_cards.empty()) {
		m_visible_index = 0;
		m_target_index = 0;
		return;
	}

	if (m_visible_index < 0 || m_visible_index >= (int)m_cards.size()) {
		m_visible_index = 0;
	}

	if (m_target_index < 0 || m_target_index >= (int)m_cards.size()) {
		m_target_index = m_visible_index;
	}
}

void CardStack::nextCard()
{
	int index;

	if (m_cards.empty()) {
		return;
	}

	index = getIndex() + 1;

	if (index >= (int)m_cards.size()) {
		index = 0;
	}

	showIndex(index);
}

void CardStack::prevCard()
{
	int index;

	if (m_cards.empty()) {
		return;
	}

	index = getIndex() - 1;

	if (index < 0) {
		index = (int)m_cards.size() - 1;
	}

	showIndex(index);
}

void CardStack::showIndex(int index)
{
	beginTransition(index);
}

void CardStack::setIndex(int index)
{
	finishTransitionTo(index);
}

void CardStack::show(const char *name)
{
	size_t i;

	if (name == nullptr || m_cards.empty()) {
		return;
	}

	for (i = 0; i < m_card_names.size(); i++) {
		if (m_card_names[i].compare(name) == 0) {
			showIndex((int)i);
			return;
		}
	}
}

int CardStack::getIndex() const
{
	if (m_transition != CARDSTACK_TRANSITION_NONE) {
		return m_target_index;
	}

	return m_visible_index;
}

int CardStack::isTransitioning() const
{
	return m_transition != CARDSTACK_TRANSITION_NONE;
}

void CardStack::draw(pvr_list_type_t list)
{
	Scene *card;

	clampVisibleIndex();

	if (m_background_rect != nullptr && !m_background_rect->isFinished()) {
		m_background_rect->draw(list);
	}

	if (m_background_banner != nullptr && !m_background_banner->isFinished()) {
		m_background_banner->draw(list);
	}

	if (!m_cards.empty()) {
		card = m_cards[m_visible_index];

		if (card != nullptr) {
			card->draw(list);
		}
	}
}

void CardStack::nextFrame()
{
	Scene *card;

	clampVisibleIndex();
	advanceTransition();

	if (m_background_rect != nullptr && !m_background_rect->isFinished()) {
		m_background_rect->nextFrame();
	}

	if (m_background_banner != nullptr && !m_background_banner->isFinished()) {
		m_background_banner->nextFrame();
	}

	if (!m_cards.empty()) {
		card = m_cards[m_visible_index];

		if (card != nullptr) {
			card->nextFrame();
		}
	}
}

extern "C"
{

CardStack *TSU_CardStackCreate(float width, float height)
{
	return new CardStack(width, height);
}

void TSU_CardStackDestroy(CardStack **cardstack_ptr)
{
	if (cardstack_ptr != NULL && *cardstack_ptr != NULL) {
		delete *cardstack_ptr;
		*cardstack_ptr = NULL;
	}
}

int TSU_CardStackCheck(Drawable *drawable_ptr)
{
	if (drawable_ptr != NULL && drawable_ptr->getObjectType() == ObjectTypeEnum::CARDSTACK_TYPE) {
		return 1;
	}

	return 0;
}

void TSU_CardStackAddCard(CardStack *cardstack_ptr, Scene *scene_ptr, const char *name)
{
	if (cardstack_ptr != NULL) {
		cardstack_ptr->addCard(scene_ptr, name);
	}
}

void TSU_CardStackSetBackgroundColor(CardStack *cardstack_ptr, const Color *color, pvr_list_type_t list, float z)
{
	if (cardstack_ptr != NULL && color != NULL) {
		cardstack_ptr->setBackgroundColor(*color, list, z);
	}
}

void TSU_CardStackSetBackground(CardStack *cardstack_ptr, Banner *banner_ptr)
{
	if (cardstack_ptr != NULL) {
		cardstack_ptr->setBackground(banner_ptr);
	}
}

void TSU_CardStackSetFadeStep(CardStack *cardstack_ptr, float step)
{
	if (cardstack_ptr != NULL) {
		cardstack_ptr->setFadeStep(step);
	}
}

void TSU_CardStackNext(CardStack *cardstack_ptr)
{
	if (cardstack_ptr != NULL) {
		cardstack_ptr->nextCard();
	}
}

void TSU_CardStackPrev(CardStack *cardstack_ptr)
{
	if (cardstack_ptr != NULL) {
		cardstack_ptr->prevCard();
	}
}

void TSU_CardStackShowIndex(CardStack *cardstack_ptr, int index)
{
	if (cardstack_ptr != NULL) {
		cardstack_ptr->showIndex(index);
	}
}

void TSU_CardStackSetIndex(CardStack *cardstack_ptr, int index)
{
	if (cardstack_ptr != NULL) {
		cardstack_ptr->setIndex(index);
	}
}

void TSU_CardStackShow(CardStack *cardstack_ptr, const char *name)
{
	if (cardstack_ptr != NULL) {
		cardstack_ptr->show(name);
	}
}

int TSU_CardStackGetIndex(CardStack *cardstack_ptr)
{
	if (cardstack_ptr != NULL) {
		return cardstack_ptr->getIndex();
	}

	return 0;
}

int TSU_CardStackIsTransitioning(CardStack *cardstack_ptr)
{
	if (cardstack_ptr != NULL) {
		return cardstack_ptr->isTransitioning();
	}

	return 0;
}

}
