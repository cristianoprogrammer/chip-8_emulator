//
//  GUI_View.cpp
//  GUI_View
//
//  Created by Panutat Tejasen on 21/12/2561 BE.
//  Copyright © 2561 Jimmy Software Co., Ltd. All rights reserved.
//

#include "GUI_View.h"
#include "GUI_Utils.h"
#include "GUI_shapes.h"
#include <list>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

extern int GUI_physicalWindowWidth;
extern int GUI_physicalWindowHeight;

extern int GUI_windowWidth;
extern int GUI_windowHeight;

extern SDL_Renderer *GUI_renderer;
extern float GUI_scale;
extern float GUI_mouseScale;

extern GUI_View *GUI_topView;
//extern GUI_View * GUI_mouseCapturedView;

GUI_View *GUI_View::lastInteractView = NULL;
GUI_View *GUI_View::lastFocusView = NULL;

std::vector<GUI_View *>GUI_View::closeQueue;

GUI_View *GUI_View::create( GUI_View *parent, const char *title, int x, int y, int width, int height,
                               std::function<bool(SDL_Event* ev)>userEventHandler) {
    GUI_View *view = new GUI_View( parent, title, x, y, width, height, userEventHandler );
    return view;
}

GUI_View::GUI_View( GUI_View *p, const char *t, int x, int y, int w, int h,
                   std::function<bool(SDL_Event* ev)>userEventHandler):
parent(p),
children(0),
topLeft(x * GUI_scale, y * GUI_scale),
rectView(x * GUI_scale, y * GUI_scale, w == -1 ? w : w * GUI_scale, h == -1 ? h : h * GUI_scale),
rectClip(0,0,0,0),
_backgroundColor(cWhite),
_borderColor(cBlack),
_textColor(cBlack),
focusBorder(0),
dragable(false),
drag_limit(false),
focusable(false),
clickable(false),
capture_on_click(false),
click_to_top(false),
_padding{0,0,0,0},
_margin{0,0,0,0},
_layout(GUI_LAYOUT_ABSOLUTE),
_align(GUI_ALIGN_LEFT | GUI_ALIGN_TOP),
_contentAlign(GUI_ALIGN_VCENTER | GUI_ALIGN_CENTER),
lastMousePoint(0, 0),
_visible(true),
_enable(true),
_focus(false),
_interact(false),
_dragging(false),
showInteract(false),
mouseReceive(true),
callback(nullptr),
isMoving(false),
focus_need_input(false),
isMouseCapturing(false),
callback_on_mouse_up(false),
callback_on_mouse_down(false),
callback_on_drag(false),
propagate_sibling_on_mouseup_outside(true),
drag_outside_parent(false),
in_scroll_bed(false),
_saftyMarginFlag(0),
_saftyPaddingFlag(0),
textSelectionScrollIndex(0),
textSelectionStartIndex(0),
textSelectionEndIndex(0)
{
    ox = x;
    oy = y;
    ow = w;
    oh = h;
    
    setBorder(1);
    setCorner(0);
    
    if (t) {
        title = std::string(t);
    }
    if (parent) {
        parent->add_child(this);
    }
}

GUI_View::~GUI_View() {
    //GUI_Log( "Kill %s\n", title.c_str() );
    
    delete_all_children();
    
    if( parent ) {
        parent->remove_child(this);
    }
}

void GUI_View::setUserEventHandler( std::function<bool(SDL_Event* ev)>handler ) {
    user_events_handler = handler;
}

void GUI_View::update() {
    if( isMoving ) {
        int moveTime = SDL_GetTicks()-moveTimeStart;
        if( moveTime >= moveDuration ) {
            isMoving = false;
            moveTime = moveDuration;
        }
        int x = moveOriginX + ((moveTargetX-moveOriginX) * (float)moveTime / (float)moveDuration);
        int y = moveOriginY + ((moveTargetY-moveOriginY) * (float)moveTime / (float)moveDuration);
        
        int dx = x - topLeft.x;
        int dy = y - topLeft.y;
        
        move_topLeft( dx, dy );
    }
}

bool GUI_View::eventHandler(SDL_Event*event) {
    if (user_events_handler) {
        if (user_events_handler(event))
            return true;;
    }
    bool needAfterUpdateLayout = false;
    bool ReverseRecursive = false;
    bool BreakSiblingPropagate = false;
    bool BreakRecursive = false;
    switch( event->type ) {
        case GUI_UpdateSize:
            //GUI_Log( "UpdateSize: %s\n", title.c_str() );
            topLeft.x = ox * GUI_scale;
            topLeft.y = oy * GUI_scale;
            rectView.x = ox * GUI_scale;
            rectView.y = oy * GUI_scale;
            rectView.w = ow == -1 ? ow : ow * GUI_scale;
            rectView.h = oh == -1 ? oh : oh * GUI_scale;
            
            setSaftyPadding();
            setSaftyMargin();
            
            if( parent ) {
                parent->updateLayout();
            }
            else {
                updateLayout();
            }
            needAfterUpdateLayout = true;
            break;
        case GUI_EventUpdate:
            update();
            break;
        case GUI_EventPaint:
            if( isVisible() ) {
                predraw();
                draw();
                SDL_Rect rClip;
                SDL_Rect rView;
                SDL_RenderGetViewport(GUI_renderer, &rView);
                SDL_RenderGetClipRect(GUI_renderer, &rClip);
                for (int it = 0 ; it < children.size(); ++it) {
                    GUI_View *child = children[it];
                    if( child->eventHandler(event) )
                        return true;
                }
                SDL_RenderSetViewport(GUI_renderer, &rView);
                SDL_RenderSetClipRect(GUI_renderer, &rClip);
                postdraw();
                BreakRecursive = true;
            }
            else {
                BreakRecursive = true;
            }
            break;
#if defined(__IPHONEOS__) || defined(__ANDROID__) || defined(__RPI__)

        case SDL_FINGERDOWN:
        {
            //GUI_Log( "Finger DOWN %s\n", title.c_str() );
            SDL_TouchFingerEvent e = event->tfinger;
            SDL_FingerID fid = e.fingerId;
            
            int x = (int)(e.x*GUI_windowWidth*GUI_mouseScale);
            int y = (int)(e.y*GUI_windowHeight*GUI_mouseScale);

            if( !mouseReceive ) {
                //GUI_Log( "Throuth %s\n", title.c_str() );
                BreakRecursive = true;
                BreakSiblingPropagate = false;
                break;
            }
            ReverseRecursive = true;

            //GUI_Log( "X,Y: %i, %i\n", x, y );
            if( hitTest(x, y, false) ) {
                //GUI_Log( "Hit %s\n", title.c_str() );
                if( mouseReceive ) {
                    if( parent && parent->_dragging ) {
                        parent->_dragging = false;
                    }
                }
                if( focusable ) {
                    setFocus();
                }
                if( click_to_top ) {
                    if( toTop() ) {
                        BreakSiblingPropagate = true;
                    }
                }
                if( callback_on_mouse_down ) {
                    if( callback ) {
                        callback(this);
                    }
                }
                if( clickable || capture_on_click ) {
                    GUI_SetMouseCapture(this);
                    touchTime = SDL_GetTicks(); // time in millis
                    touchHoldTime = touchTime;
                    BreakSiblingPropagate = true;
                }
                if( dragable ) {
                    //GUI_Log( "Drag %s\n", title.c_str() );
                    GUI_SetMouseCapture(this);
                    _dragging = true;
                    if( parent ) {
                        parent->_dragging = false;
                    }
                    BreakSiblingPropagate = true;
                }
                lastMousePoint.set(x, y);
            }
            else {
                return false;
            }
            break;
        }
#endif
#if !defined(__IPHONEOS__) && !defined(__ANDROID__) && !defined(__RPI__)

        case SDL_MOUSEBUTTONDOWN:
        {
            //GUI_Log( "Mouse DOWN %s\n", title.c_str() );

            SDL_MouseButtonEvent e = event->button;
            
            int x = (int)(e.x*GUI_mouseScale);
            int y = (int)(e.y*GUI_mouseScale);

            if( !mouseReceive ) {
                //GUI_Log( "Throuth %s\n", title.c_str() );
                BreakRecursive = true;
                BreakSiblingPropagate = false;
                break;
            }
            ReverseRecursive = true;
            
            if( hitTest(x, y, false) ) {
                //GUI_Log( "Hit %s\n", title.c_str() );
                if( mouseReceive ) {
                    if( parent && parent->_dragging ) {
                        parent->_dragging = false;
                    }
                }
                if( focusable ) {
                    setFocus();
                }
                if( click_to_top ) {
                    if( toTop() ) {
                        BreakSiblingPropagate = true;
                    }
                }
                if( callback_on_mouse_down ) {
                    if( callback ) {
                        callback(this);
                    }
                }
                if( clickable || capture_on_click ) {
                    GUI_SetMouseCapture(this);
                    touchTime = SDL_GetTicks(); // time in millis
                    touchHoldTime = touchTime;
                    BreakSiblingPropagate = true;
                }
                if( dragable ) {
                    //GUI_Log( "Drag %s\n", title.c_str() );
                    GUI_SetMouseCapture(this);
                    _dragging = true;
                    if( parent ) {
                        parent->_dragging = false;
                    }
                    BreakSiblingPropagate = true;
                }
                lastMousePoint.set(x, y);
            }
            else {
                return false;
            }
            break;
        }
#endif
#if defined(__IPHONEOS__) || defined(__ANDROID__) || defined(__RPI__)


        case SDL_FINGERMOTION:
        {
            SDL_TouchFingerEvent e = event->tfinger;
            SDL_FingerID fid = e.fingerId;
            //GUI_Log( "%i\n", fid );
            
            int x = (int)(e.x*GUI_windowWidth*GUI_mouseScale);
            int y = (int)(e.y*GUI_windowHeight*GUI_mouseScale);
            
            if( !mouseReceive ) {
                BreakRecursive = true;
                BreakSiblingPropagate = false;
                break;
            }
            
            ReverseRecursive = true;
            
            if (_dragging) {
                //GUI_Log( "Finger DRAGGING %s\n", title.c_str() );
                if (parent) {
                    if (!parent->hitTest(x, y, false) && !(drag_outside_parent)) {
                        setInteract( false );
                        return true;
                    }
                }
                setInteract( true );
                int dx = x - lastMousePoint.x;
                int dy = y - lastMousePoint.y;
                if( drag_limit ) {
                    int tlX = topLeft.x + dx;
                    int tlY = topLeft.y + dy;
                    
                    if( tlY < dragMinY * GUI_scale )
                        tlY = dragMinY * GUI_scale;
                    if( tlY > dragMaxY * GUI_scale )
                        tlY = dragMaxY * GUI_scale;
                    
                    if( tlX < dragMinX * GUI_scale )
                        tlX = dragMinX * GUI_scale;
                    if( tlX > dragMaxX * GUI_scale )
                        tlX = dragMaxX * GUI_scale;
                    
                    dy = tlY - topLeft.y;
                    dx = tlX - topLeft.x;
                }
                lastMousePoint.set(x, y);
                move_topLeft(dx, dy);
                if( callback_on_drag && callback ) {
                    callback( this );
                }
                return true;
            }
            if( hitTest(x, y, false) ) {
                //GUI_Log( "Finger MOTION %s\n", title.c_str() );

                int dx = x - lastMousePoint.x;
                int dy = y - lastMousePoint.y;
                lastMousePoint.set(x, y);
                int dragThreshold = 2;
#ifdef __ANDROID__
                dragThreshold = 4;
#endif
                if( in_scroll_bed && isFocus() && ((dx >= 0 ? dx : -dx) > dragThreshold || (dy >= 0 ? dy : -dy) > dragThreshold)) {
                    if( parent && parent->dragable ) {
                        //GUI_Log( "Refer DRAG to Parent %s\n", title.c_str() );
                        parent->_dragging = true;
                        parent->setFocus();
                    }
                }
                //GUI_Log( "Mouse motion %s\n", title.c_str() );
                
                BreakSiblingPropagate = true;
                setInteract( true );
                if( parent ) {
                    parent->setInteract( false );
                }
            }
            else {
                setInteract( false );
                BreakRecursive = true;
            }
            break;
        }
#endif
#if !defined(__IPHONEOS__) && !defined(__ANDROID__) && !defined(__RPI__)
        case SDL_MOUSEMOTION:
        {
            SDL_MouseMotionEvent e = event->motion;
            
            int x = (int)(e.x*GUI_mouseScale);
            int y = (int)(e.y*GUI_mouseScale);
            
            if( !mouseReceive ) {
                BreakRecursive = true;
                BreakSiblingPropagate = false;
                break;
            }

            ReverseRecursive = true;

            if (_dragging) {
                //GUI_Log( "Mouse DRAGGING %s\n", title.c_str() );
                if (parent) {
                    if (!parent->hitTest(x, y, false) && !(drag_outside_parent)) {
                        setInteract( false );
                        return true;
                    }
                }
                setInteract( true );
                int dx = x - lastMousePoint.x;
                int dy = y - lastMousePoint.y;
                if( drag_limit ) {
                    int tlX = topLeft.x + dx;
                    int tlY = topLeft.y + dy;
                    
                    if( tlY < dragMinY * GUI_scale )
                        tlY = dragMinY * GUI_scale;
                    if( tlY > dragMaxY * GUI_scale )
                        tlY = dragMaxY * GUI_scale;
                    
                    if( tlX < dragMinX * GUI_scale )
                        tlX = dragMinX * GUI_scale;
                    if( tlX > dragMaxX * GUI_scale )
                        tlX = dragMaxX * GUI_scale;
                    
                    dy = tlY - topLeft.y;
                    dx = tlX - topLeft.x;
                }
                lastMousePoint.set(x, y);
                move_topLeft(dx, dy);
                if( callback_on_drag && callback ) {
                    callback( this );
                }
                return true;
            }
            if( hitTest(x, y, false) ) {
                //GUI_Log( "Mouse MOTION IN %s\n", title.c_str() );

                int dx = x - lastMousePoint.x;
                int dy = y - lastMousePoint.y;
                lastMousePoint.set(x, y);
                int dragThreshold = 2;
#ifdef __ANDROID__
                dragThreshold = 2;
#endif
                if( in_scroll_bed && isFocus() && ((dx >= 0 ? dx : -dx) > dragThreshold || (dy >= 0 ? dy : -dy) > dragThreshold)) {
                    if( parent && parent->dragable ) {
                        //GUI_Log( "Refer DRAG to Parent %s\n", title.c_str() );
                        parent->_dragging = true;
                        parent->setFocus();
                    }
                }
                //GUI_Log( "Mouse motion %s\n", title.c_str() );

                BreakSiblingPropagate = true;
                setInteract( true );
                if( parent ) {
                    parent->setInteract( false );
                }
            }
            else {
                setInteract( false );
                BreakRecursive = true;
            }
            break;
        }
#endif
#if defined(__IPHONEOS__) || defined(__ANDROID__) || defined(__RPI__)

        case SDL_FINGERUP:
        {

            SDL_TouchFingerEvent e = event->tfinger;
            //SDL_FingerID fid = e.fingerId;
            //GUI_Log( "%i\n", fid );
            //SDL_MouseButtonEvent e = event->button;
            int x = (int)(e.x*GUI_windowWidth*GUI_mouseScale);
            int y = (int)(e.y*GUI_windowHeight*GUI_mouseScale);
            
            if( !mouseReceive ) {
                BreakRecursive = true;
                BreakSiblingPropagate = false;
                break;
            }
            if (_dragging) {
                _dragging = false;
                SDL_Log( "Undragging %s\n", title.c_str() );
                //GUI_mouseCapturedView = NULL;
                GUI_SetMouseCapture(NULL);
                return true;
            }
            
            if( hitTest(x, y, false) ) {
                //GUI_Log( "Finger UP IN %s\n", title.c_str() );
                BreakSiblingPropagate = true;
                if( in_scroll_bed ) {
                    if( parent && parent->dragable ) {
                        parent->_dragging = false;
                        parent->setFocus();
                        GUI_SetMouseCapture(NULL);
                    }
                }
                if( clickable ) {
                    if( isMouseCapturing )
                        GUI_SetMouseCapture(NULL);
                    if (callback && !callback_on_mouse_up) {
                        callback(this);
                    }
                    return true;
                }
                if( callback_on_mouse_up ) {
                    if( callback ) {
                        callback(this);
                    }
                    return true;
                }
            }
            else {
                //GUI_Log( "Finger UP OUT %s\n", title.c_str() );
                if( isFocus() ) {
                    killFocus();
                }
                if( isMouseCapturing ) {
                    GUI_SetMouseCapture(NULL);
                }
                if( !propagate_sibling_on_mouseup_outside ) {
                    BreakSiblingPropagate = true;
                }
                BreakRecursive = true;
            }
            
            break;
        }
#endif
#if !defined(__IPHONEOS__) && !defined(__ANDROID__) && !defined(__RPI__)
        case SDL_MOUSEBUTTONUP:
        {

            SDL_MouseButtonEvent e = event->button;
            int x = (int)(e.x*GUI_mouseScale);
            int y = (int)(e.y*GUI_mouseScale);
            
            if( !mouseReceive ) {
                BreakRecursive = true;
                BreakSiblingPropagate = false;
                break;
            }
            if (_dragging) {
                _dragging = false;
                SDL_Log( "Undragging %s\n", title.c_str() );
                //GUI_mouseCapturedView = NULL;
                GUI_SetMouseCapture(NULL);
                return true;
            }
            
            if( hitTest(x, y, false) ) {
                //GUI_Log( "Mouse UP IN %s\n", title.c_str() );
                BreakSiblingPropagate = true;
                if( in_scroll_bed ) {
                    if( parent && parent->dragable ) {
                        parent->_dragging = false;
                        parent->setFocus();
                        GUI_SetMouseCapture(NULL);
                    }
                }
                if( clickable ) {
                    if( isMouseCapturing )
                        GUI_SetMouseCapture(NULL);
                    if (callback && !callback_on_mouse_up) {
                        callback(this);
                    }
                    return true;
                }
                if( callback_on_mouse_up ) {
                    if( callback ) {
                        callback(this);
                    }
                    return true;
                }
            }
            else {
                //GUI_Log( "Mouse UP OUT %s\n", title.c_str() );
                if( isFocus() ) {
                    killFocus();
                }
                if( isMouseCapturing ) {
                    GUI_SetMouseCapture(NULL);
                }
                if( !propagate_sibling_on_mouseup_outside ) {
                    BreakSiblingPropagate = true;
                }
                BreakRecursive = true;
            }
            
            break;
        }
#endif
        case SDL_TEXTINPUT:
        {
            if( !isFocus() ) {
                if( lastFocusView ) {
                    lastFocusView->eventHandler(event);
                    return true;
                }
            }
            return true;
        }
        default:
            break;
    }
    if( !BreakRecursive ) {
        if( ReverseRecursive ) {
            //GUI_Log( "Reverse traverse\n" );
            if( children.size() > 0 ) {
                for (int n = (int)children.size()-1; n >= 0; --n) {
                    GUI_View *child = children[n];
                    if( child->eventHandler(event) )
                        return true;
                }
            }
        }
        else {
            for (int it = 0 ; it < children.size(); ++it) {
                GUI_View *child = children[it];
                if( child->eventHandler(event) )
                    return true;
            }
        }
        if( needAfterUpdateLayout ) {
            updateLayout();
        }
    }

    return BreakSiblingPropagate;
}

void GUI_View::move_topLeft(int dx, int dy) {
    topLeft.x += dx;
    topLeft.y += dy;
    
    rectView.x += dx;
    rectView.y += dy;
    
    if( _align & GUI_ALIGN_ABSOLUTE ) {
        //ox = topLeft.x;
        //oy = topLeft.y;
    }
    
    for (int it = 0 ; it < children.size(); ++it) {
        GUI_View *child = children[it];
        child->move_rectView(dx, dy);
    }
}

void GUI_View::move_rectView(int dx, int dy) {
    rectView.x += dx;
    rectView.y += dy;
    
    for (int it = 0 ; it < children.size(); ++it) {
        GUI_View *child = children[it];
        child->move_rectView(dx, dy);
    }
}

void GUI_View::move( int dx, int dy, int time ) {
    if( time == 0 ) {
        move_topLeft( dx * GUI_scale, dy * GUI_scale );
    }
    else {
        moveOriginX = topLeft.x;
        moveOriginY = topLeft.y;
        moveTargetX = topLeft.x + (dx * GUI_scale);
        moveTargetY = topLeft.y + (dy * GUI_scale);
        moveDuration = time;
        moveTimeStart = SDL_GetTicks();
        isMoving = true;
    }
}

void GUI_View::moveTo( int x, int y, int time ) {
    int dx = x - rectView.x / GUI_scale;
    int dy = y - rectView.y / GUI_scale;
    
    move( dx, dy, time );
}

void GUI_View::add_child(GUI_View *child) {
    if( child->parent ) {
        child->parent->remove_child( child );
    }
    child->parent = this;
    
    children.push_back(child);
    
    updateLayout();
}

void GUI_View::remove_child(GUI_View *child) {
    for (int it = 0 ; it < children.size(); ++it) {
        if( child == children[it] ) {
            children.erase(children.begin()+ it );
            // GUI_Log( "Remove %s\n", child->title.c_str() );
            if( child->isMouseCapturing )
                GUI_SetMouseCapture( NULL );
            if( lastInteractView == child )
                lastInteractView = NULL;
            if( lastFocusView == child )
                lastFocusView = NULL;
            child->parent = NULL;
            break;
        }
    }
    updateLayout();
}

void GUI_View::close() {
    GUI_View::closeQueue.push_back(this);
}

void GUI_View::delete_all_children() {
    //children.clear();
    while( children.size() ) {
        GUI_View *child = children.at(0);
        remove_child( child );
        delete(child);
    }
    //children.clear();
}


void GUI_View::predraw() {
    if (isVisible() == false)
        return;

    int f = 0;
    if( isFocus() && focusBorder > 0 ) {
        f = focusBorder * GUI_scale;
        
        if (parent) {
            GUI_Rect parent_clip = GUI_Rect(parent->rectClip);
            parent_clip.x -= topLeft.x;
            parent_clip.y -= topLeft.y;
            SDL_IntersectRect(GUI_MakeRect(0, 0, rectView.w+(f*2), rectView.h+(f*2)), &parent_clip, &rectClip);
            
            // Bug of SDL
            if (rectClip.w < 0)
                rectClip.w = 0;
            if (rectClip.h < 0)
                rectClip.h = 0;
        } else {
            rectClip = GUI_Rect(0, 0, rectView.w, rectView.h);
        }
        
#ifdef __EMSCRIPTENx__
        SDL_RenderSetViewport(GUI_renderer, GUI_MakeRect(rectView.x, GUI_windowHeight*GUI_scale - rectView.y - rectView.h, rectView.w, rectView.h));
#else
        SDL_RenderSetViewport(GUI_renderer, GUI_MakeRect(rectView.x-f, rectView.y-f, rectView.w+(f*2), rectView.h+(f*2)));
#endif
        
#ifdef __EMSCRIPTENx__
        float magic_y = GUI_windowHeight*GUI_scale - rectView.y - rectView.h;
        
        SDL_RenderSetClipRect(GUI_renderer, GUI_MakeRect(rectView.x + rectClip.x,
                                                         0 - magic_y + rectClip.y,
                                                         rectClip.w,
                                                         rectClip.h));
#else
        SDL_RenderSetClipRect(GUI_renderer, GUI_MakeRect(rectClip.x,
                                                         rectClip.y,
                                                         rectClip.w,
                                                         rectClip.h));
#endif
        drawFocus();
    }
    
    if (parent) {
        GUI_Rect parent_clip = GUI_Rect(parent->rectClip);
        parent_clip.x -= topLeft.x;
        parent_clip.y -= topLeft.y;
        SDL_IntersectRect(GUI_MakeRect(0, 0, rectView.w, rectView.h), &parent_clip, &rectClip);
        
        // Bug of SDL
        if (rectClip.w < 0)
            rectClip.w = 0;
        if (rectClip.h < 0)
            rectClip.h = 0;
    } else {
        rectClip = GUI_Rect(0, 0, rectView.w, rectView.h);
    }

#ifdef __EMSCRIPTENx__
    SDL_RenderSetViewport(GUI_renderer, GUI_MakeRect(rectView.x, GUI_windowHeight*GUI_scale - rectView.y - rectView.h, rectView.w, rectView.h));
#else
    SDL_RenderSetViewport(GUI_renderer, GUI_MakeRect(rectView.x, rectView.y, rectView.w, rectView.h));
#endif

#ifdef __EMSCRIPTENx__
    float magic_y = GUI_windowHeight*GUI_scale - rectView.y - rectView.h;
    
    SDL_RenderSetClipRect(GUI_renderer, GUI_MakeRect(rectView.x + rectClip.x,
                                                     0 - magic_y + rectClip.y,
                                                     rectClip.w,
                                                     rectClip.h));
#else
    SDL_RenderSetClipRect(GUI_renderer, GUI_MakeRect(rectClip.x,
                                                     rectClip.y,
                                                     rectClip.w,
                                                     rectClip.h));
#endif
    clear();
}

void GUI_View::draw() {

}

void GUI_View::drawInteract() {
    if( showInteract && getInteract() ) {
        GUI_Rect *rect = GUI_MakeRect(0, 0, rectView.w, rectView.h);
        
        if (getCorner() != 0) {
            GUI_FillRoundRect(rect->x, rect->y, rect->w, rect->h, getCorner() * GUI_scale, cHightLightInteract);
        } else {
            GUI_FillRect(rect->x, rect->y, rect->w, rect->h, cHightLightInteract);
        }
    }
}

void GUI_View::postdraw() {
    drawInteract();
    int border = getBorder();
    int corner = getCorner();
    
    if (border > 0) {
        GUI_Rect r = GUI_Rect(border, border, rectView.w - (2 * border), rectView.h - (2 * border));
        
        if (corner) {
            GUI_DrawRoundRect(0, 0, rectView.w, rectView.h, corner * GUI_scale, _borderColor);
        } else {
            GUI_DrawRect(0, 0, rectView.w, rectView.h, _borderColor);
        }
        
        SDL_IntersectRect(&r, &rectClip, &rectClip);
        
#ifdef __EMSCRIPTENx__
        float magic_y = GUI_windowHeight*GUI_scale - rectView.y - rectView.h;
        
        SDL_RenderSetClipRect(GUI_renderer, GUI_MakeRect(rectView.x + rectClip.x,
                                                         0 - magic_y + rectClip.y,
                                                         rectClip.w,
                                                         rectClip.h));
#else
        SDL_RenderSetClipRect(GUI_renderer, GUI_MakeRect(rectClip.x,
                                                         rectClip.y,
                                                         rectClip.w,
                                                         rectClip.h));
#endif
    }

}

void GUI_View::clear(GUI_Rect *rect) {
    if (!rect)
        rect = GUI_MakeRect(0, 0, rectView.w, rectView.h);
    
    if (_backgroundColor.a != 0) {
        int corner = getCorner();
        if (corner != 0) {
            GUI_FillRoundRect(rect->x, rect->y, rect->w, rect->h, corner * GUI_scale, _backgroundColor);
        } else {
            GUI_FillRect(rect->x, rect->y, rect->w, rect->h, _backgroundColor);
        }
    }
}

void GUI_View::drawFocus() {
    if (!isFocus())
        return;
    if (focusBorder <= 0 )
        return;
    
    GUI_Rect *rect = GUI_MakeRect(0, 0, rectView.w+(focusBorder*GUI_scale*2), rectView.h+(focusBorder*GUI_scale*2));
    
    if (_backgroundColor.a != 0) {
        SDL_Color color = _backgroundColor;
        color.a = 96;
        color.r &= 0x7F;
        color.g &= 0x7F;
        color.b &= 0x7F;
        int corner = getCorner();
        if (corner != 0) {
            GUI_FillRoundRect(rect->x, rect->y, rect->w, rect->h, ((corner) * GUI_scale)+focusBorder*GUI_scale, color);
        } else {
            GUI_FillRect(rect->x, rect->y, rect->w, rect->h, color);
        }
    }
}

bool GUI_View::toTop() {
    if( !parent )
        return false;
    
    for (int it = 0 ; it < parent->children.size(); ++it) {
        if( this == parent->children[it] ) {
            parent->children.erase(parent->children.begin() + it );
            parent->children.push_back(this);
            return true;
        }
    }
    return false;
}

bool GUI_View::toBack() {
    if( !parent )
        return false;
    
    for (int it = 0 ; it < parent->children.size(); ++it) {
        if( this == parent->children[it] ) {
            parent->children.erase(parent->children.begin()+ it );
            parent->children.insert(parent->children.begin(), this);
            return true;
        }
    }
    return false;
}


void GUI_View::setInteract(bool i) {
    if( i ) {
        // GUI_Log( "Interact %s\n", title.c_str() );
        if( lastInteractView != NULL && lastInteractView != this ) {
            lastInteractView->setInteract(false);
        }
        _interact = true;
        lastInteractView = this;
    }
    else {
        _interact = false;
        //GUI_Log( "DE-Interact %s\n", title.c_str() );
    }
}

void GUI_View::setFocus() {
    if( lastFocusView == this )
        return;
    if( lastFocusView != NULL && lastFocusView != this ) {
        lastFocusView->killFocus();
    }
    lastFocusView = this;
    _focus = true;
    //GUI_Log( "Focus: %s\n", title.c_str() );
    if( focus_need_input ) {
#ifdef __EMSCRIPTEN__
        //emscripten_run_script("document.getElementById('canvas').contentEditable = true;");
#endif
        SDL_StartTextInput();
    }
};

void GUI_View::killFocus() {
    if( lastFocusView == this ) {
        lastFocusView = NULL;
        if( focus_need_input ) {
            SDL_StopTextInput();
        }
    }
    _focus = false;
};

void GUI_View::setPadding(int p0, int p1, int p2, int p3) {
    _padding[0] = p0;
    _padding[1] = p1;
    _padding[2] = p2;
    _padding[3] = p3;
    
    updateSize();
    this->updateLayout();
}

void GUI_View::setMargin(int p0, int p1, int p2, int p3) {
    _margin[0] = p0;
    _margin[1] = p1;
    _margin[2] = p2;
    _margin[3] = p3;
    
    if (parent) {
        parent->updateLayout();
        updateLayout();
    }
    this->updateLayout();
}

GUI_View *GUI_View::hitTest(int x, int y, bool bRecursive) {
    if (!isVisible()) {
        //GUI_Log( "Invisible %s\n", title.c_str() );
        return 0;
    }
    
    SDL_Point pt;
    pt.x = x;
    pt.y = y;
    
    if (SDL_PointInRect(&pt, &rectView)) {
        // GUI_Log( "Hit in %s\n", title.c_str() );
        if (bRecursive) {
            if( children.size() > 0 ) {
                for (int it = children.size()-1 ; it >= 0; --it) {
                    GUI_View *child = children[it];
                    //GUI_Log( "Hit2\n" );
                    
                    GUI_View *wb = child->hitTest(x, y, bRecursive);
                    if (wb) {
                        //GUI_Log( "Hitted %s\n", wb->title.c_str());
                        return wb;
                    }
                }
            }
        }
        return this;
    }
    return 0;
}

void GUI_View::updateSize() {
}

void GUI_View::updateLayout() {
    //GUI_Log( "UpdateLayout %s\n", title.c_str() );
    updateSize();
    
    if( this == GUI_topView ) {
        if (ox == 0)
            rectView.x = _margin[3] * GUI_scale;
        
        if (oy == 0)
            rectView.y = _margin[0] * GUI_scale;
        
        if (ow == -1)
            rectView.w = (GUI_windowWidth - _margin[1]) * GUI_scale - rectView.x;
        
        if (oh == -1)
            rectView.h = (GUI_windowHeight - _margin[0]) * GUI_scale - rectView.y;
    }
    if (_layout == GUI_LAYOUT_ABSOLUTE) {
        bool sz_changed = false;
        for (std::vector<GUI_View *>::iterator it = children.begin() ; it != children.end(); ++it) {
            GUI_View *child = *it;
            
            if (child->isVisible() == false)
                continue;
            
            if (child->_align & GUI_ALIGN_RIGHT) {
                child->topLeft.x = rectView.w - child->rectView.w - (child->_margin[1] + _padding[1]) * GUI_scale;
                child->rectView.x = rectView.x + child->topLeft.x;
            } else if (child->_align & GUI_ALIGN_CENTER) {
                if (child->ow == -1) {
                    child->rectView.w = rectView.w - child->topLeft.x - (child->_margin[1] + _padding[1]) * GUI_scale;
                }
                child->topLeft.x = (rectView.w - child->rectView.w) / 2;
                child->rectView.x = rectView.x + child->topLeft.x;
            } else {
                if ((child->_align & GUI_ALIGN_ABSOLUTE) == 0) { // Left Align || ow == 0
                    if (child->ox == 0) {
                        child->topLeft.x = (child->_margin[3] + _padding[3]) * GUI_scale;
                    } else {
                        child->topLeft.x = child->ox * GUI_scale;
                    }
                }
                
                child->rectView.x = rectView.x + child->topLeft.x;
                
                if (child->ow == -1) {
                    child->rectView.w = rectView.w - child->topLeft.x - (child->_margin[1] + _padding[1]) * GUI_scale;
                }
            }
            
            if (ow == 0) {
                int wwx = (child->rectView.w + (_padding[1] + _padding[3] + child->_margin[1] + child->_margin[3])*GUI_scale); // + (border * 2);
                
                if (wwx != rectView.w) {
                    rectView.w = wwx;
                    sz_changed = true;
                }
            }
            
            if (child->_align & GUI_ALIGN_BOTTOM) {
                child->topLeft.y = rectView.h - child->rectView.h - (child->_margin[2] + _padding[2]) * GUI_scale;
                child->rectView.y = rectView.y + child->topLeft.y;
            } else if (child->_align & GUI_ALIGN_VCENTER) {
                if (child->oh == -1) {
                    child->rectView.h = rectView.h - child->topLeft.y - (child->_margin[2] + _padding[2]) * GUI_scale;
                }
                child->topLeft.y = (rectView.h - child->rectView.h) / 2;
                child->rectView.y = rectView.y + child->topLeft.y;
            } else {
                if ((child->_align & GUI_ALIGN_ABSOLUTE) == 0) { // Top Align || oh == 0
                    if (child->oy == 0) {
                        child->topLeft.y = (child->_margin[0] + _padding[0]) * GUI_scale;
                    } else {
                        child->topLeft.y = child->oy * GUI_scale;
                    }
                }
                
                child->rectView.y = rectView.y + child->topLeft.y;
                
                if (child->oh == -1) {
                    child->rectView.h = rectView.h - child->topLeft.y - (child->_margin[2] + _padding[2]) * GUI_scale;
                }
            }
            
            if (oh == 0) {
                int hhx = (child->rectView.h + (_padding[0] + _padding[2] + child->_margin[0] + child->_margin[2])*GUI_scale); // + (border * 2);
                
                if (hhx != rectView.h) {
                    rectView.h = hhx;
                    sz_changed = true;
                }
            }
            
            if (ow == 0 || oh == 0) {
                if (sz_changed) {
                    int oww = ow;
                    int ohh = oh;
                    ow = rectView.w;
                    oh = rectView.h;
                    //updateSubLayout();
                    
                    if (_align & (GUI_ALIGN_CENTER | GUI_ALIGN_RIGHT | GUI_ALIGN_BOTTOM | GUI_ALIGN_VCENTER)) {
                        if (parent) {
                            parent->updateLayout();
                        }
                    }
                    
                    ow = oww;
                    oh = ohh;
                }
            }
            
            child->updateLayout();
        }
    }
    if (_layout == GUI_LAYOUT_HORIZONTAL) {
        int x = 0;
        
        x += _padding[3] * GUI_scale;
        
        int w = 0;
        int h = 0;
        
        for (int it = 0 ; it < children.size(); ++it) {
            GUI_View *child = children[it];
            
            if (child->isVisible() == false)
                continue;
            
            if (child->_align & GUI_ALIGN_ABSOLUTE) {
                //child->topLeft.x = child->ox * GUI_scale;
                //child->topLeft.y = child->oy * GUI_scale;
                child->rectView.x = rectView.x + child->topLeft.x;
                child->rectView.y = rectView.y + child->topLeft.y;
                
                if (child->oh == -1) {
                    child->rectView.h = rectView.h - (_padding[2] + child->_margin[2] + _padding[0] + child->_margin[0]) * GUI_scale;
                }
                if (child->ow == -1) {
                    child->rectView.w = rectView.w - (_padding[1] + child->_margin[1] + _padding[3] + child->_margin[3]) * GUI_scale;
                }
            } else if (child->_align & GUI_ALIGN_CENTER) {
                child->topLeft.x = (rectView.w - child->rectView.w) / 2;
                child->rectView.x = rectView.x + child->topLeft.x;
                
                for (int iit = it-1; iit >= 0; --iit) {
                    GUI_View *cc = children[iit];
                    
                    if (cc->isVisible() == false)
                        continue;
                    
                    if (cc->_align & GUI_ALIGN_CENTER) {
                        cc->topLeft.x -= child->rectView.w / 2 + ((child->_margin[3] + cc->_margin[1]) * GUI_scale) / 2;
                        cc->rectView.x = rectView.x + cc->topLeft.x;
                        child->topLeft.x += cc->rectView.w / 2 + ((child->_margin[3] + cc->_margin[1]) * GUI_scale) / 2;
                        child->rectView.x = rectView.x + child->topLeft.x;
                        cc->updateLayout();
                    }
                }
            } else if (child->_align & GUI_ALIGN_RIGHT) {
                child->topLeft.x = (rectView.w - child->rectView.w) - ((child->_margin[1] + _padding[1]) * GUI_scale);
                child->rectView.x = rectView.x + child->topLeft.x;
                
                for (int iit = it-1; iit >= 0; --iit) {
                    GUI_View *cc = children[iit];
                    
                    if (cc->isVisible() == false)
                        continue;
                    
                    if (cc->_align & GUI_ALIGN_RIGHT) {
                        cc->topLeft.x -= child->rectView.w + (child->_margin[1] + child->_margin[3]) * GUI_scale;
                        cc->rectView.x = rectView.x + cc->topLeft.x;
                        
                        cc->updateLayout();
                    } else if (cc->_align & GUI_ALIGN_CENTER) {
                    } else {
                        if (cc->ow == -1) {
                            cc->rectView.w -= child->rectView.w + (child->_margin[1] + child->_margin[3]) * GUI_scale;
                            cc->ow = -2;
                            cc->updateLayout();
                            cc->ow = -1;
                            break;
                        }
                    }
                }
            } else {
                child->topLeft.x = x + child->_margin[3] * GUI_scale;
                child->rectView.x = rectView.x + child->topLeft.x;
                
                if (child->ow == -1) {
                    child->rectView.w = rectView.w - child->topLeft.x - (_padding[1] + child->_margin[1])* GUI_scale;
                }
                else {
                    for (int iit = it-1; iit >= 0; --iit) {
                        GUI_View *cc = children[iit];
                        
                        if (cc->isVisible() == false)
                            continue;
                        
                        if (cc->_align & GUI_ALIGN_RIGHT) {
                        } else if (cc->_align & GUI_ALIGN_CENTER) {
                        } else {
                            if (cc->ow == -1) {
                                int cur_width = child->rectView.w + (child->_margin[1] + child->_margin[3]) * GUI_scale;
                                cc->rectView.w -= cur_width;
                                cc->ow = -2;
                                cc->updateLayout();
                                cc->ow = -1;
                                x -= cur_width;
                                for (int iiit = iit+1; iiit < it; iiit++) {
                                    GUI_View *ccc = children[iiit];
                                    ccc->topLeft.x -= cur_width;
                                    ccc->rectView.x = rectView.x + ccc->topLeft.x;
                                    ccc->updateLayout();
                                }
                                //child->topLeft.y -= child->rectView.h + child->_margin[2] * GUI_scale;
                                //child->rectView.y = rectView.y + child->topLeft.y;
                                child->topLeft.x = x + child->_margin[3] * GUI_scale;
                                child->rectView.x = rectView.x + child->topLeft.x;
                                break;
                            }
                        }
                    }
                }
                
                x += child->rectView.w + (child->_margin[1] + child->_margin[3]) * GUI_scale;
            }
            
            w += child->rectView.w + (child->_margin[3] + child->_margin[1]) * GUI_scale;
            
            int hh = child->rectView.h + (child->_margin[0] + child->_margin[2]) * GUI_scale;
            if( hh > h ) {
                h = hh;
            }
            
            if (child->_align & GUI_ALIGN_ABSOLUTE) {
            } else if (child->_align & GUI_ALIGN_BOTTOM) {
                child->topLeft.y = rectView.h - child->rectView.h - (_padding[2] + child->_margin[2])*GUI_scale;
                child->rectView.y = rectView.y + child->topLeft.y;
            } else if (child->_align & GUI_ALIGN_VCENTER) {
                child->topLeft.y = (rectView.h - child->rectView.h) / 2;
                child->rectView.y = rectView.y + child->topLeft.y;
            } else {
                child->topLeft.y = (_padding[0] + child->_margin[0]) * GUI_scale;
                child->rectView.y = rectView.y + child->topLeft.y;
                
                if (child->oh == -1) {
                    child->rectView.h = rectView.h - (_padding[0] + child->_margin[0] + _padding[2] + child->_margin[2]) * GUI_scale;
                }
            }
            child->updateLayout();
        }
        w += (_padding[1] + _padding[3]) * GUI_scale; // + (border * 2);
        h += (_padding[0] + _padding[2]) * GUI_scale; // + (border * 2);
        if( ow == 0 ) {
            if( rectView.w != w ) {
                rectView.w = w;
                if( parent && parent->ow == 0 ) {
                    parent->updateLayout();
                }
            }
        }
        if( oh == 0 ) {
            if( rectView.h != h ) {
                rectView.h = h;
                if( parent && parent->oh == 0 ) {
                    parent->updateLayout();
                }
            }
        }
    } else if (_layout == GUI_LAYOUT_VERTICAL) {
        int y = 0;
        
        y += _padding[0] * GUI_scale;
        
        int w = 0;
        int h = 0;
        
        for (int it = 0 ; it < children.size(); ++it) {
            GUI_View *child = children[it];
            
            if (child->isVisible() == false)
                continue;
            
            if (child->_align & GUI_ALIGN_ABSOLUTE) {
                //child->topLeft.x = child->ox * GUI_scale;
                //child->topLeft.y = child->oy * GUI_scale;
                child->rectView.x = rectView.x + child->topLeft.x;
                child->rectView.y = rectView.y + child->topLeft.y;
                
                if (child->oh == -1) {
                    child->rectView.h = rectView.h - (_padding[2] + child->_margin[2] + _padding[0] + child->_margin[0]) * GUI_scale;
                }
                if (child->ow == -1) {
                    child->rectView.w = rectView.w - (_padding[1] + child->_margin[1] + _padding[3] + child->_margin[3]) * GUI_scale;
                }
            } else if (child->_align & GUI_ALIGN_VCENTER) {
                child->topLeft.y = (rectView.h - child->rectView.h) / 2;
                child->rectView.y = rectView.y + child->topLeft.y;
                
                for (int iit = it-1; iit >= 0; --iit) {
                    GUI_View *cc = children[iit];
                    
                    if (cc->isVisible() == false)
                        continue;
                    
                    if (cc->_align & GUI_ALIGN_VCENTER) {
                        cc->topLeft.y -= child->rectView.h / 2 + ((child->_margin[0] + cc->_margin[2]) * GUI_scale) / 2;
                        cc->rectView.y = rectView.y + cc->topLeft.y;
                        child->topLeft.y += cc->rectView.h / 2 + ((child->_margin[0] + cc->_margin[2]) * GUI_scale) / 2;
                        child->rectView.y = rectView.y + child->topLeft.y;
                        cc->updateLayout();
                    }
                }
            } else if (child->_align & GUI_ALIGN_BOTTOM) {
                child->topLeft.y = (rectView.h - child->rectView.h) - ((child->_margin[2] + _padding[2]) * GUI_scale);
                child->rectView.y = rectView.y + child->topLeft.y;
                
                for (int iit = it-1; iit >= 0; --iit) {
                    GUI_View *cc = children[iit];
                    
                    if (cc->isVisible() == false)
                        continue;
                    
                    if (cc->_align & GUI_ALIGN_BOTTOM) {
                        cc->topLeft.y -= child->rectView.h + (child->_margin[0] + child->_margin[2]) * GUI_scale;
                        cc->rectView.y = rectView.y + cc->topLeft.y;
                        
                        cc->updateLayout();
                    } else if (cc->_align & GUI_ALIGN_VCENTER) {
                    } else {
                        if (cc->oh == -1) {
                            cc->rectView.h -= child->rectView.h + (child->_margin[0] + child->_margin[2]) * GUI_scale;
                            cc->oh = -2;
                            cc->updateLayout();
                            cc->oh = -1;
                            break;
                        }
                    }
                }
            } else {
                child->topLeft.y = y + child->_margin[0] * GUI_scale;
                child->rectView.y = rectView.y + child->topLeft.y;
                
                if (child->oh == -1) {
                    child->rectView.h = rectView.h - child->topLeft.y - (_padding[2] + child->_margin[2])* GUI_scale;
                }
                else {
                    for (int iit = it-1; iit >= 0; --iit) {
                        GUI_View *cc = children[iit];
                        
                        if (cc->isVisible() == false)
                            continue;
                        
                        if (cc->_align & GUI_ALIGN_BOTTOM) {
                        } else if (cc->_align & GUI_ALIGN_VCENTER) {
                        } else {
                            if (cc->oh == -1) {
                                int cur_height = child->rectView.h + (child->_margin[0] + child->_margin[2]) * GUI_scale;
                                cc->rectView.h -= cur_height;
                                cc->oh = -2;
                                cc->updateLayout();
                                cc->oh = -1;
                                y -= cur_height;
                                for (int iiit = iit+1; iiit < it; iiit++) {
                                    GUI_View *ccc = children[iiit];
                                    ccc->topLeft.y -= cur_height;
                                    ccc->rectView.y = rectView.y + ccc->topLeft.y;
                                    ccc->updateLayout();
                                }
                                //child->topLeft.y -= child->rectView.h + child->_margin[2] * GUI_scale;
                                //child->rectView.y = rectView.y + child->topLeft.y;
                                child->topLeft.y = y + child->_margin[0] * GUI_scale;
                                child->rectView.y = rectView.y + child->topLeft.y;
                                break;
                            }
                        }
                    }
                }
                
                y += child->rectView.h + (child->_margin[0] + child->_margin[2]) * GUI_scale;
            }

            h += child->rectView.h + (child->_margin[0] + child->_margin[2]) * GUI_scale;

            int ww = child->rectView.w + (child->_margin[1] + child->_margin[3]) * GUI_scale;
            if( ww > w ) {
                w = ww;
            }
            
            if (child->_align & GUI_ALIGN_ABSOLUTE) {

            } else if (child->_align & GUI_ALIGN_RIGHT) {
                child->topLeft.x = rectView.w - child->rectView.w - (_padding[1] + child->_margin[1])*GUI_scale;
                child->rectView.x = rectView.x + child->topLeft.x;
            } else if (child->_align & GUI_ALIGN_CENTER) {
                child->topLeft.x = (rectView.w - child->rectView.w) / 2;
                child->rectView.x = rectView.x + child->topLeft.x;
            } else {
                child->topLeft.x = (_padding[3] + child->_margin[3]) * GUI_scale;
                child->rectView.x = rectView.x + child->topLeft.x;
                
                if (child->ow == -1) {
                    child->rectView.w = rectView.w - (_padding[1] + child->_margin[1] + _padding[3] + child->_margin[3]) * GUI_scale;
                }
            }
            child->updateLayout();
        }
        w += (_padding[1] + _padding[3]) * GUI_scale; // + (border * 2);
        h += (_padding[0] + _padding[2]) * GUI_scale; // + (border * 2);
        if( ow == 0 ) {
            if( rectView.w != w ) {
                rectView.w = w;
                if( parent && parent->ow == 0 ) {
                    parent->updateLayout();
                }
            }
        }
        if( oh == 0 ) {
            if( rectView.h != h ) {
                rectView.h = h;
                if( parent && parent->oh == 0 ) {
                    parent->updateLayout();
                }
            }
        }
    }
}

void GUI_View::setAbsolutePosition( int x, int y, int duration ) {
    int dx = x - rectView.x/GUI_scale;
    int dy = y - rectView.y/GUI_scale;
        
    move( dx, dy, duration );
}

GUI_Point GUI_View::getAbsolutePosition() {
    static GUI_Point pt;
    
    pt.x = rectView.x / GUI_scale;
    pt.y = rectView.y / GUI_scale;
    
    return pt;
}

void GUI_View::setCorner( int c ) {
    _corner = c;
}

void GUI_View::setBorder( int b ) {
    _border = b;
}

void GUI_View::setAlign( int a ) {
    _align = a;
    if( a & GUI_ALIGN_ABSOLUTE ) {
        topLeft.x = ox * GUI_scale;
        topLeft.y = oy * GUI_scale;
    }

    if( parent )
        parent->updateLayout();
};

void GUI_View::setSaftyMarginFlag( int flag ) {
    _saftyMarginFlag = flag;
    setSaftyMargin();
}

void GUI_View::setSaftyPaddingFlag( int flag ) {
    _saftyPaddingFlag = flag;
    setSaftyPadding();
}

void GUI_View::setSaftyMargin() {
#ifdef __IPHONEOS__
    int *saftyMargins = getContentSaftyMargin();
#else
    int saftyMargins[4] = { 0, 0, 0, 0 };
#endif

    if( _saftyMarginFlag ) {
        if( _saftyMarginFlag & 1 ) { _margin[0] = saftyMargins[0]; }
        if( _saftyMarginFlag & 2 ) { _margin[1] = saftyMargins[1]; }
        if( _saftyMarginFlag & 4 ) { _margin[2] = saftyMargins[2]; }
        if( _saftyMarginFlag & 8 ) { _margin[3] = saftyMargins[3]; }

        if (parent) {
            parent->updateLayout();
            updateLayout();
        }
        this->updateLayout();
    }
}

void GUI_View::setSaftyPadding() {
#ifdef __IPHONEOS__
    int *saftyPaddings = getContentSaftyMargin();
#else
    int saftyPaddings[4] = { 0, 0, 0, 0 };
#endif
    
    if( _saftyPaddingFlag ) {
        if( _saftyPaddingFlag & 1 ) { _padding[0] = saftyPaddings[0]; }
        if( _saftyPaddingFlag & 2 ) { _padding[1] = saftyPaddings[1]; }
        if( _saftyPaddingFlag & 4 ) { _padding[2] = saftyPaddings[2]; }
        if( _saftyPaddingFlag & 8 ) { _padding[3] = saftyPaddings[3]; }
        
        if (parent) {
            parent->updateLayout();
            updateLayout();
        }
        this->updateLayout();
    }
}

void GUI_View::printf( const char * format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    setTitle( std::string(buffer) );
}

bool GUI_View::textSelectionIsSelected() {
    return (textSelectionStartIndex != textSelectionEndIndex);
}

void GUI_View::textSelectionGetSelectedIndex(int* startIndex, int* endIndex) {
    *startIndex = textSelectionStartIndex;
    *endIndex = textSelectionEndIndex;

    if (*endIndex < *startIndex) {
        *startIndex = textSelectionEndIndex;
        *endIndex = textSelectionStartIndex;
    }
}

void GUI_View::textSelectionCancel() {
    textSelectionScrollIndex = 0;
    textSelectionStartIndex = 0;
    textSelectionEndIndex = textSelectionStartIndex;
}
