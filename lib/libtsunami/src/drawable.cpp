/*
   Tsunami for KallistiOS ##version##

   drawable.cpp

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera
   
*/

#include "drawable.h"
#include <plx/matrix.h>

#include <algorithm>
#include "tsunamiutils.h"
// #include <kos.h>

// Constructor / Destructor
Drawable::Drawable() {
	m_trans.zero();
	m_rotate.zero();
	m_scale = Vector(1.0f, 1.0f, 1.0f, 1.0f);
	m_alpha = 1.0f;

	m_t_prelative = true;
	m_r_prelative = true;
	m_s_prelative = true;
	m_a_prelative = true;

	m_finished = false;
	m_subs_finished = false;

	m_parent = nullptr;
}

Drawable::~Drawable() {
}

void Drawable::animAdd(Animation *ani) {
	m_anims.push_front(ani);
}

void Drawable::animRemove(Animation *ani) {
	auto is_ptr = [=](Animation *sp) { return sp == ani; };
	auto it = std::find_if(m_anims.begin(), m_anims.end(), is_ptr);

	if (it != m_anims.end())
		m_anims.erase(it);
}

void Drawable::animRemoveAll() {
	m_anims.clear();
}

bool Drawable::isFinished() {
	if (!m_subs_finished) {
		for (auto it: m_subs) {
			if (!it->isFinished())
				return false;
		}

		m_subs_finished = true;
	}

	return m_finished;
}

void Drawable::setFinished() {
	m_finished = true;

	for (auto it: m_subs) {
		it->setFinished();
	}
}

void Drawable::subDraw(int list) {
	for (auto it: m_subs) {
		if (!it->isFinished())
			it->draw(list);
	}
}

void Drawable::subNextFrame() {
	for (auto it: m_subs) {
		if (!it->isFinished())
			it->nextFrame();
	}
}

void Drawable::subAdd(Drawable *t) {
	assert( t->m_parent == nullptr );
	t->m_parent = this;
	m_subs.push_front(t);
}

void Drawable::subRemove(Drawable *t) {
	t->m_parent = nullptr;

	auto is_ptr = [=](Drawable *sp) { return sp == t; };
	auto it = std::find_if(m_subs.begin(), m_subs.end(), is_ptr);

	if (it != m_subs.end())
		m_subs.erase(it);
}

void Drawable::subRemoveFinished() {
	for (auto it = m_subs.begin(); it != m_subs.end();) {
		if ((*it)->isFinished()) {
			(*it)->m_parent = nullptr;
			it = m_subs.erase(it);
		} else {
			it++;
		}
	}
}

void Drawable::subRemoveAll() {
	for (auto it = m_subs.begin(); it != m_subs.end();) {
		(*it)->m_parent = nullptr;
		it = m_subs.erase(it);
	}
}

void Drawable::draw(int list) {
	subDraw(list);
}

void Drawable::nextFrame() {
	/* Duplicate the array of animations. This makes the "for" loop much
	 * easier as we don't have to handle it->nextFrame() calling
	 * animRemove(). */
	auto anims = m_anims;

	for (auto it: anims) {
		it->nextFrame(this);
	}

	subNextFrame();
}

Vector Drawable::getPosition() const {
	Vector pos = getTranslate();
	if (m_parent)
		pos += m_parent->getPosition();

	return pos;
}

Color Drawable::getColor() const {
	Color tint = getTint();
	if (m_parent)
		tint *= m_parent->getColor();

	return tint;
}

void Drawable::pushTransformMatrix() const {
	const Vector & pos = getTranslate();
	const Vector & rot = getRotate();
	const Vector & scale = getScale();

	plx_mat3d_push();
	plx_mat3d_translate(pos.x, pos.y, pos.z);
	plx_mat3d_rotate(rot.w, rot.x, rot.y, rot.z);
	plx_mat3d_scale(scale.x, scale.y, scale.z);
}

void Drawable::popTransformMatrix() const {
	plx_mat3d_pop();
}


extern "C" 
{
	void TSU_DrawableAnimAdd(Drawable *drawable_ptr, Animation *anim_ptr)
	{
		if (drawable_ptr != NULL && anim_ptr != NULL) {
			drawable_ptr->animAdd(anim_ptr);
		}
	}

	void TSU_DrawableAnimRemove(Drawable *drawable_ptr, Animation *anim_ptr)
	{
		if (drawable_ptr != NULL && anim_ptr != NULL) {
			drawable_ptr->animRemove(anim_ptr);
		}
	}

	void TSU_DrawableAnimRemoveAll(Drawable *drawable_ptr)
	{
		if (drawable_ptr != NULL) {
			drawable_ptr->animRemoveAll();
		}
	}

	bool TSU_DrawableIsFinished(Drawable *drawable_ptr)
	{
		if (drawable_ptr != NULL) {
			return drawable_ptr->isFinished();
		}
		else {
			return false;
		}
	}

	void TSU_DrawableSubDraw(Drawable *drawable_ptr, int list)
	{
		if (drawable_ptr != NULL) {
			drawable_ptr->subDraw(list);
		}
	}

	void TSU_DrawableSubNextFrame(Drawable *drawable_ptr)
	{
		if (drawable_ptr != NULL) {
			drawable_ptr->subNextFrame();
		}
	}

	void TSU_DrawableSubAdd(Drawable *drawable_ptr, Drawable *new_drawable_ptr)
	{
		if (drawable_ptr != NULL && new_drawable_ptr != NULL) {
			drawable_ptr->subAdd(new_drawable_ptr);
		}
	}

	void TSU_DrawableSubRemove(Drawable *drawable_ptr, Drawable *remove_drawable_ptr)
	{
		if (drawable_ptr != NULL && remove_drawable_ptr != NULL) {
			drawable_ptr->subRemove(remove_drawable_ptr);
			delete remove_drawable_ptr;
			remove_drawable_ptr = NULL;
		}
	}

	void TSU_DrawableSubRemoveFinished(Drawable *drawable_ptr)
	{
		if (drawable_ptr != NULL) {
			drawable_ptr->subRemoveFinished();
		}
	}

	void TSU_DrawableSubRemoveAll(Drawable *drawable_ptr)
	{
		if (drawable_ptr != NULL) {
			drawable_ptr->subRemoveAll();
		}
	}

	void TSU_DrawableSetTranslate(Drawable *drawable_ptr, const Vector *v)
	{
		if (drawable_ptr != NULL) {
			drawable_ptr->setTranslate(*v);
		}
	}

	const Vector* TSU_DrawableGetTranslate(Drawable *drawable_ptr)
	{
		return &drawable_ptr->getTranslate();
	}

	void TSU_DrawableTranslate(Drawable *drawable_ptr, const Vector *v)
	{
		if (drawable_ptr != NULL) {
			drawable_ptr->translate(*v);
		}
	}

	Vector TSU_DrawableGetPosition(Drawable *drawable_ptr)
	{
		return drawable_ptr->getPosition();
	}

	void TSU_DrawableSetRotate(Drawable *drawable_ptr, const Vector *r)
	{
		if (drawable_ptr != NULL) {
			drawable_ptr->setRotate(*r);
		}
	}

	const Vector* TSU_DrawableGetRotate(Drawable *drawable_ptr)
	{
		return &drawable_ptr->getRotate();
	}

	void TSU_DrawableSetScale(Drawable *drawable_ptr, const Vector *s)
	{
		if (drawable_ptr != NULL) {
			drawable_ptr->setScale(*s);
		}
	}

	const Vector* TSU_DrawableGetScale(Drawable *drawable_ptr)
	{
		return &drawable_ptr->getScale();
	}

	void TSU_DrawableSetTint(Drawable *drawable_ptr, const Color *tint)
	{
		if (drawable_ptr != NULL) {
			drawable_ptr->setTint(*tint);
		}
	}

	const Color* TSU_DrawableGetTint(Drawable *drawable_ptr)
	{
		return &drawable_ptr->getTint();
	}

	void TSU_DrawableSetAlpha(Drawable *drawable_ptr, float a)
	{
		if (drawable_ptr != NULL) {
			drawable_ptr->setAlpha(a);
		}
	}

	float TSU_DrawableGetAlpha(Drawable *drawable_ptr)
	{
		if (drawable_ptr != NULL) {
			return drawable_ptr->getAlpha();
		}
		else {
			return 0;
		}
	}

	Color TSU_DrawableGetColor(Drawable *drawable_ptr)
	{
		return drawable_ptr->getColor();
	}

	Drawable* TSU_DrawableGetParent(Drawable *drawable_ptr)
	{
		return drawable_ptr->getParent();
	}
}