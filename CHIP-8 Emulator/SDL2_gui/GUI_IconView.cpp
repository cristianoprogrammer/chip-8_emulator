//
//  GUI_IconView.cpp
//  GUI_TextView
//
//  Created by Panutat Tejasen on 12/1/2562 BE.
//  Copyright © 2562 Jimmy Software Co., Ltd. All rights reserved.
//

#include "GUI_IconView.h"
#include "GUI_Fonts.h"

GUI_IconView *GUI_IconView::create( GUI_View *parent, uint16_t unicode, const char *fontname, int fontsize, int x, int y, int width, int height,
                                   std::function<bool(SDL_Event* ev)>userEventHandler ) {
    return new GUI_IconView(parent, unicode, fontname, fontsize, x, y, width, height, userEventHandler );
}

GUI_IconView::GUI_IconView(GUI_View *parent, uint16_t unicode, const char *fontname, int fontsize, int x, int y, int width, int height,
                           std::function<bool(SDL_Event* ev)>userEventHandler) :
GUI_TextView(parent, NULL, fontname, fontsize, x, y, width, height, userEventHandler )
{
    mouseReceive = false;

    setIcon(unicode);
}

GUI_IconView::~GUI_IconView() {
    
}

void GUI_IconView::updateContent() {
    SDL_Texture *texture = createTextureFormUnicode( icon );
    if (texture == NULL){
        GUI_Log("Could not create icon texture\n");
        return;
    }
    image.setTexture(texture);
    
    updateSize();
    if( parent ) {
        parent->updateLayout();
    }
    else {
        updateLayout();
    }
}

void GUI_IconView::setIcon( uint16_t unicode ) {
    icon = unicode;
    updateContent();
}

SDL_Texture* GUI_IconView::createTextureFormUnicode(Uint16 unicode, SDL_Rect* rect) {
    if (font) {
        SDL_Surface* surf = TTF_RenderGlyph_Blended(font, unicode, cWhite);
        SDL_Texture *tex = SDL_CreateTextureFromSurface(GUI_renderer, surf);
        
        if (rect != NULL) {
            rect->x = rect->y = 0;
            rect->w = surf->w;
            rect->h = surf->h;
        }
        
        SDL_FreeSurface(surf);
        return tex;
    }
    
    return NULL;
}

bool GUI_IconView::eventHandler(SDL_Event*event) {
    switch (event->type) {
        case GUI_FontChanged:
        {
            std::string fn;
            
            fn = _fontName;
            
            int fs = _fontSize;
            if( fs == 0 ) {
                fs = GUI_GetUIIconFontSize();
            }
            
            std::string fontPath = std::string("data/")+fn;
            font = GUI_Fonts::getFont(fn, fs);
            
            updateContent();
            updateSize();
            
            break;
        }
        default:
        {
            return GUI_ImageView::eventHandler(event);
        }
    }
    return false;
}
