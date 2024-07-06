/*
   Tsunami for KallistiOS ##version##

   drawable.h

   Copyright (C) 2002 Megan Potter

*/

#ifndef __TSUNAMI_DRAWABLE_H
#define __TSUNAMI_DRAWABLE_H

#include "animation.h"

#include "vector.h"
#include "color.h"

#include <deque>
#include <memory>

class Drawable {
public:
	/// Constructor / Destructor
	Drawable();
	virtual ~Drawable();

	/// Add an animation object to us
	void animAdd(std::shared_ptr<Animation> ani);

	/// Remove an animation object from us
	void animRemove(Animation * ani);

	/// Remove all animation objects from us
	void animRemoveAll();

	/// Checks to see if this object is still not finished (for screen
	/// closing type stuff). Returns true if this object and all
	/// sub-objects are finished.
	bool isFinished();

	/// Set this object to be finished
	virtual void setFinished();

	/// Draw all sub-drawables (if any)
	void subDraw(int list);

	/// Move to all sub-drawables to the next frame (if any)
	void subNextFrame();

	/// Add a new object to our sub-drawables
	void subAdd(std::shared_ptr<Drawable> t);

	/// Remove an object from our sub-drawables
	void subRemove(Drawable * t);

	/// Remove any sub-drawables that are marked finished
	void subRemoveFinished();

	/// Remove all objects from our sub-drawables
	void subRemoveAll();

	/// Draw this drawable for the given list
	virtual void draw(int list);

	/// Move to the next frame of animation
	virtual void nextFrame();

	/// Modify the drawn position of this drawable
	void setTranslate(const Vector & v) { m_trans = v; }

	/// Get the drawn position of this drawable
	const Vector & getTranslate() const { return m_trans; }

	/// Move this drawable relative to where it is now
	void translate(const Vector & v) { m_trans += v; }

	/// Get the absolute position of this drawable (figuring prelative)
	Vector getPosition() const;

	/// Modify the rotation of this drawable; the angle is
	/// stored as the w value.
	void setRotate(const Vector & r) { m_rotate = r; }

	/// Get the rotation of this drawable; the angle is
	/// stored as the w value.
	const Vector & getRotate() const { return m_rotate; }

	/// Modify the scaling of this drawable
	void setScale(const Vector & s) { m_scale = s; }

	/// Get the scaling of this drawable
	const Vector & getScale() const { return m_scale; }

	/// Set the color tint value of this drawable
	void setTint(const Color & tint) { m_tint = tint; }

	/// Get the color tint value of this drawable
	const Color & getTint() const { return m_tint; }

	/// Shortcut to set the alpha value of the tint
	void setAlpha(float a) { m_tint.a = a; }

	/// Shortcut to get the alpha value of the tint
	float getAlpha() const { return m_tint.a; }

	/// Get the absolute tint value of this drawable (figuring prelative)
	Color getColor() const;

	/// Get our parent object (if any)
	Drawable * getParent() const { return m_parent; }

protected:
	/// Setup a transform matrix, taking into account the
	/// parent relative rotation and scaling parameters. Pushes the old
	/// matrix onto the stack.
	void pushTransformMatrix() const;

	/// Pops the old matrix off the stack.
	void popTransformMatrix() const;

private:
	Vector		m_trans;		///< Translation
	Vector		m_rotate;		///< Rotation (w is the angle)
	Vector		m_scale;		///< Scaling (about center)
	float		m_alpha;		///< Alpha value
	Color		m_tint;			///< Color tint value
	bool		m_t_prelative;		///< Is translation parent-relative?
	bool		m_r_prelative;		///< Is rotation parent-relative?
	bool		m_s_prelative;		///< Is scaling parent-relative?
	bool		m_a_prelative;		///< Is alpha parent-relative?

	bool		m_finished;		///< Is it "finished" (i.e., can safely be removed from scene)
	bool		m_subs_finished;	///< Cached resultes if all sub-drawables are finished

	Drawable	* m_parent;		///< Our parent object

	std::deque<std::shared_ptr<Animation>>	m_anims;		///< Animation objects
	std::deque<std::shared_ptr<Drawable>>	m_subs;			///< Our sub-drawable list
};

#endif	/* __TSUNAMI_DRAWABLE_H */
