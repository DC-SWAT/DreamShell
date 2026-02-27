/*
   Tsunami for KallistiOS ##version##

   drawable.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024-2026 Maniac Vera
   Copyright (C) 2026 SWAT

*/

#ifndef __TSUNAMI_DRAWABLE_H
#define __TSUNAMI_DRAWABLE_H

#include "inputeventstate.h"
#include "animation.h"

#include "vector.h"
#include "color.h"

typedef void (*ClickEventFunctionPtr)(Drawable *drawable);
typedef void (*OnMouseOverEventFunctionPtr)(Drawable *drawable, uint object_type, int id);

enum ObjectTypeEnum
{
	NONE_TYPE = 0,
	DRAWABLE_TYPE = 1<<0,
	SCENE_TYPE = 1<<2,
	FORM_TYPE = 1<<3,
	BANNER_TYPE = 1<<4,
	LABEL_TYPE = 1<<5,
	ITEMMENU_TYPE = 1<<6,
	OPTIONGROUP_TYPE = 1<<7,
	TEXTBOX_TYPE = 1<<8,
	COMBOBOX_TYPE = 1<<9,
	CHECKBOX_TYPE = 1<<10,
	RECTANGLE_TYPE = 1<<11,
	BOX_TYPE = 1<<12,
	TRIANGLE_TYPE = 1<<13,
	CIRCLE_TYPE = 1<<14,
	GRADIENT_TYPE = 1<<15,
	WAVE_TYPE = 1<<16
};

#ifdef __cplusplus

#include <deque>
#include <memory>

class Drawable : public InputEventState {
public:
	/// Constructor / Destructor
	Drawable();
	virtual ~Drawable();

	/// Add an animation object to us
	void animAdd(Animation *ani);

	/// Remove an animation object from us
	void animRemove(Animation *ani);

	/// Remove all animation objects from us
	void animRemoveAll();

	void inputEvent(int event_type, int key) {} 

	/// Checks to see if this object is still not finished (for screen
	/// closing type stuff). Returns true if this object and all
	/// sub-objects are finished.
	bool isFinished();

	/// Set this object to be finished
	virtual void setFinished();

	/// Draw all sub-drawables (if any)
	void subDraw(pvr_list_type_t list);

	/// Move to all sub-drawables to the next frame (if any)
	void subNextFrame();

	/// Add a new object to our sub-drawables
	void subAdd(Drawable *t);

	/// Remove an object from our sub-drawables
	void subRemove(Drawable * t);

	/// Remove any sub-drawables that are marked finished
	void subRemoveFinished();

	/// Remove all objects from our sub-drawables
	void subRemoveAll();

	/// Draw this drawable for the given list
	virtual void draw(pvr_list_type_t list);

	/// Move to the next frame of animation
	virtual void nextFrame();

	virtual void onMouseOver();	

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

	virtual void setSize(float width, float height) { 
		if (width > 0) {
			m_width = width;
		}

		if (height > 0) {
			m_height = height;
		}
	}

	virtual void getSize(float *width, float *height) { 
		*width = m_width;
		*height = m_height;
	}
	
	void setReadOnly(bool is_readonly) { m_readonly = is_readonly; }

	bool isReadOnly() { return m_readonly; }

	void setId(uint32 id) { m_id = id; }
	int getId() { return m_id; }

	virtual void click();
	void setClickEvent(ClickEventFunctionPtr click_function);
	void setOnMouseOverEvent(OnMouseOverEventFunctionPtr mouse_over_function);

	uint getObjectType();
	uint getObjectSubType();
	static bool isMouseInside(Drawable *drawable, int mx, int my);

protected:
	/// Setup a transform matrix, taking into account the
	/// parent relative rotation and scaling parameters. Pushes the old
	/// matrix onto the stack.
	void pushTransformMatrix() const;

	/// Pops the old matrix off the stack.
	void popTransformMatrix() const;

	void setObjectType(uint type);
	void setObjectSubType(uint subType);

	float		m_width, m_height;
	OnMouseOverEventFunctionPtr m_mouse_over_function;

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
	bool		m_readonly;
	int			m_id;
	uint		m_object_type;
	uint		m_object_subtype;

	Drawable	* m_parent;		///< Our parent object

	std::deque<Animation *>	m_anims;		///< Animation objects
	std::deque<Drawable *>	m_subs;			///< Our sub-drawable list
	ClickEventFunctionPtr m_click_function;
};

#else

#ifndef TYPEDEF_DRAWABLE
	typedef struct drawable Drawable;
#endif

#endif


#ifdef __cplusplus
extern "C"
{
#endif

	void TSU_DrawableEventSetClick(Drawable *drawable_ptr, ClickEventFunctionPtr click_function);
	void TSU_DrawableEventSetOnMouseOver(Drawable *drawable_ptr, OnMouseOverEventFunctionPtr mouse_over_function);
	bool TSU_DrawableIsMouseInside(Drawable *drawable_ptr, int mx, int my);
	void TSU_DrawableAnimAdd(Drawable *drawable_ptr, Animation *anim_ptr);
	void TSU_DrawableAnimRemove(Drawable *drawable_ptr, Animation *anim_ptr);
	void TSU_DrawableAnimRemoveAll(Drawable *drawable_ptr);
	bool TSU_DrawableIsFinished(Drawable *drawable_ptr);
	void TSU_DrawableSubDraw(Drawable *drawable_ptr, pvr_list_type_t list);
	void TSU_DrawableSubNextFrame(Drawable *drawable_ptr);
	void TSU_DrawableSubAdd(Drawable *drawable_ptr, Drawable *new_drawable_ptr);
	void TSU_DrawableSubRemove(Drawable *drawable_ptr, Drawable *remove_drawable_ptr);
	void TSU_DrawableSubRemoveFinished(Drawable *drawable_ptr);
	void TSU_DrawableSubRemoveAll(Drawable *drawable_ptr);
	void TSU_DrawableSetFinished(Drawable *drawable_ptr);
	void TSU_DrawableSetTranslate(Drawable *drawable_ptr, const Vector *v);
	const Vector *TSU_DrawableGetTranslate(Drawable *drawable_ptr);
	void TSU_DrawableTranslate(Drawable *drawable_ptr, const Vector *v);
	Vector TSU_DrawableGetPosition(Drawable *drawable_ptr);
	void TSU_DrawableSetRotate(Drawable *drawable_ptr, const Vector *r);
	const Vector *TSU_DrawableGetRotate(Drawable *drawable_ptr);
	void TSU_DrawableSetScale(Drawable *drawable_ptr, const Vector *s);
	const Vector *TSU_DrawableGetScale(Drawable *drawable_ptr);
	void TSU_DrawableSetTint(Drawable *drawable_ptr, const Color *tint);
	const Color *TSU_DrawableGetTint(Drawable *drawable_ptr);
	void TSU_DrawableSetAlpha(Drawable *drawable_ptr, float a);
	float TSU_DrawableGetAlpha(Drawable *drawable_ptr);
	Color TSU_DrawableGetColor(Drawable *drawable_ptr);
	Drawable* TSU_DrawableGetParent(Drawable *drawable_ptr);
	bool TSU_DrawableIsReadOnly(Drawable *drawable_ptr);
	void TSU_DrawableSetReadOnly(Drawable *drawable_ptr, bool is_readonly);
	int TSU_DrawableGetId(Drawable *drawable_ptr);
	void TSU_DrawableSetId(Drawable *drawable_ptr, int id);
	void TSU_DrawableGetSize(Drawable *drawable_ptr, float *w, float *h);
	void TSU_DrawableSetSize(Drawable *drawable_ptr, float w, float h);
	void TSU_DrawableSetWindowState(Drawable *drawable_ptr, int window_state);
	int TSU_DrawableGetWindowState(Drawable *drawable_ptr);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_DRAWABLE_H */
