/**
 * Copyright (C) 2013, Niklas Rosenstein.
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * TriStateCustomGui.cpp
 */

#include <c4d.h>
#include "../res/c4d_symbols.h"

#define CUSTOMGUI_TRISTATE 1030913

// Does not have to be unique, only used in dialog Command()
#define COMMANDMSG_VALUECHANGED 1000

// Only for the user-area, state value to draw the tri-state bitmap.
#define MODE_TRISTATE 4

static BaseBitmap* g_undef = NULL;
static BaseBitmap* g_on = NULL;
static BaseBitmap* g_off = NULL;
static BaseBitmap* g_multiple = NULL;

/**
 * Takes over the process of informing the node about
 * the new parameter value. This method invokes
 * `iCustomGui::GetData()` to retrieve the new value of the
 * parameter. The passed container should be one passed to
 * `GeDialog::Command()`.
 *
 * Take a look at `InvokeCommandCall()` for triggering the
 * `GeDialog::Command()` method intentionally from code.
 */
static Bool SendValueChanged(iCustomGui* dlg, BaseContainer msg) {
    msg.SetLong(BFM_ACTION_ID, dlg->GetId());
    msg.RemoveData(BFM_ACTION_VALUE);
    msg.SetData(BFM_ACTION_VALUE, dlg->GetData().GetValue());
    return dlg->SendParentMessage(msg);
}

/**
 * Invokes a call to the passed dialogs `Command()` method
 * with the specified ID.
 */
static Bool InvokeCommandCall(GeDialog* dialog, LONG id) {
    BaseContainer actionmsg(BFM_ACTION), actionresult;
    actionmsg.SetLong(BFM_ACTION_ID, id);
    return dialog->Message(actionmsg, actionresult);
}

// Size and padding settings for the icon.
struct {
    LONG w, h;
    LONG px, py;
} sizeinfo = { 10, 10, 2, 2 };

class TriStateUA : public GeUserArea {

public:

    void SetState(LONG state) { m_state = state; }

    LONG GetState() const { return m_state; }

    // GeUserArea Overrides

    virtual Bool GetMinSize(LONG& w, LONG& h) {
        w = sizeinfo.w + sizeinfo.px * 2;
        h = sizeinfo.h + sizeinfo.py * 2;
        return TRUE;
    }

    virtual void DrawMsg(LONG x1, LONG y1, LONG x2, LONG y2, const BaseContainer& msg) {
        SetClippingRegion(x1, y1, x2, y2);

        DrawSetPen(COLOR_BG);
        DrawRectangle(x1, y1, x2, y2);

        BaseBitmap* icon = NULL;
        switch (m_state) {
            case MODE_UNDEF:
                icon = g_undef;
                break;
            case MODE_ON:
                icon = g_on;
                break;
            case MODE_OFF:
                icon = g_off;
                break;
            case MODE_TRISTATE:
                icon = g_multiple;
                break;
            default:
                return;
        }

        if (icon) {
            DrawBitmap(icon, sizeinfo.px, sizeinfo.py, sizeinfo.w, sizeinfo.h,
                       0, 0, icon->GetBw(), icon->GetBh(), BMP_ALLOWALPHA);
        }
    }

    virtual Bool InputEvent(const BaseContainer& msg) {
        LONG device = msg.GetLong(BFM_INPUT_DEVICE);
        LONG channel = msg.GetLong(BFM_INPUT_CHANNEL);
        if (device != BFM_INPUT_MOUSE) return FALSE;

        Bool catched = FALSE;
        LONG xpos = msg.GetLong(BFM_INPUT_X);
        LONG ypos = msg.GetLong(BFM_INPUT_Y);
        Global2Local(&xpos, &ypos);
        switch (channel) {
            case BFM_INPUT_MOUSELEFT: {
                if (
                    xpos >= sizeinfo.px && xpos <= (sizeinfo.w + sizeinfo.px + 1)
                 && ypos >= sizeinfo.py && ypos <= (sizeinfo.h + sizeinfo.py + 1)
                ) {
                    m_state = (m_state + 1) % 3;
                    Redraw();
                    catched = TRUE;
                }
                break;
            }
            case BFM_INPUT_MOUSERIGHT: {
                BaseContainer menubc;
                menubc.SetString(1000 + MODE_UNDEF, GeLoadString(IDC_UNDEF));
                menubc.SetString(1000 + MODE_ON, GeLoadString(IDC_ON));
                menubc.SetString(1000 + MODE_OFF, GeLoadString(IDC_OFF));
                LONG result = ShowPopupMenu(GetDialog()->Get(), MOUSEPOS, MOUSEPOS, menubc);
                if (result >= 1000) {
                    m_state = (result - 1000) % 3;
                    Redraw();
                }
                catched = TRUE;
                break;
            }
            default:
                break;
        }
        if (catched) {
            GeDialog* dlg = GetDialog();
            InvokeCommandCall(dlg, COMMANDMSG_VALUECHANGED);
        }
        return catched;
    }

private:

    LONG m_state;

};

class TriStateGui : public iCustomGui {

    typedef iCustomGui super;

public:

    TriStateGui(const BaseContainer& settings, CUSTOMGUIPLUGIN* plug)
    : super(settings, plug) {
    }

    // iCustomGui Overrides

    virtual LONG CustomGuiWidth() {
        LONG w = 0, h;
        m_ua.GetMinSize(w, h);
        return w;
    }

    virtual LONG CustomGuiHeight() {
        LONG w, h = 0;
        m_ua.GetMinSize(w, h);
        return h;
    }

    virtual void CustomGuiRedraw() {
        m_ua.Redraw();
    }

    virtual Bool SetData(const TriState<GeData>& tristate) {
        LONG mode;
        if (tristate.GetTri()) mode = MODE_TRISTATE;
        else {
            const GeData& value = tristate.GetValue();
            switch (value.GetLong()) {
                case MODE_UNDEF:
                case MODE_ON:
                case MODE_OFF:
                    mode = value.GetLong();
                    break;
                default:
                    mode = MODE_ON;
                    break;
            }
        }
        m_ua.SetState(mode);
        m_ua.Redraw();
        return TRUE;
    }

    virtual TriState<GeData> GetData() {
        LONG mode = m_ua.GetState();
        if (mode == MODE_TRISTATE) return TriState<GeData>();
        else return TriState<GeData>(GeData(mode));
    }

    // GeDialog Overrides

    virtual Bool CreateLayout();

    virtual Bool InitValues();

    virtual Bool Command(LONG id, const BaseContainer& msg) {
        if (id == COMMANDMSG_VALUECHANGED) {
            SendValueChanged(this, msg);
        }
        return TRUE;
    }

private:

    TriStateUA m_ua;

};

Bool TriStateGui::CreateLayout() {
    AddUserArea(1000, BFH_SCALEFIT | BFV_SCALEFIT,
                sizeinfo.w + sizeinfo.px * 2,
                sizeinfo.h + sizeinfo.py * 2);
    AttachUserArea(m_ua, 1000);
    return TRUE;
}

Bool TriStateGui::InitValues() {
    return TRUE;
}

class TriStateData : public CustomGuiData {

    typedef CustomGuiData super;

public:

    // CustomGuiData Overrides

    virtual LONG GetId() { return CUSTOMGUI_TRISTATE; }

    virtual CDialog* Alloc(const BaseContainer& settings) {
        GeDialog* dlg = gNew TriStateGui(settings, GetPlugin());
        if (!dlg) return NULL;

        return dlg->Get();
    }

    virtual void Free(CDialog* dlg, void* ud) {
        if (dlg) {
            gDelete((GeDialog*&) dlg);
        }
    }

    virtual const CHAR* GetResourceSym() {
        return "TRISTATE";
    }

    virtual LONG GetResourceDataType(LONG*& table) {
        static LONG l_table[] = {
            DA_LONG,
        };
        table = l_table;
        return sizeof(l_table) / sizeof(LONG);
    }

};

BaseBitmap* LoadResourceImage(const String& name) {
    AutoBitmap abmp(name);
    BaseBitmap* bmp = abmp;
    if (bmp) return bmp->GetClone();
    return NULL;
}

Bool RegisterTriStateGui() {
    // Register a fake library. Required for recieving callbaks in iCustomGui.
    static BaseCustomGuiLib lib;
    ClearMem(&lib, sizeof(lib));
    FillBaseCustomGui(lib);
    if (!InstallLibrary(CUSTOMGUI_TRISTATE, &lib, 1000, sizeof(lib))) return FALSE;

    g_undef = LoadResourceImage("tristate-undef.tif");
    g_on = LoadResourceImage("tristate-on.tif");
    g_off = LoadResourceImage("tristate-off.tif");
    g_multiple = LoadResourceImage("tristate-multiple.png");

    return RegisterCustomGuiPlugin("Tristate", CUSTOMGUI_SUPPORT_LAYOUTDATA, gNew TriStateData);
}

void FreeTriStateGui() {
    BaseBitmap::Free(g_undef);
    BaseBitmap::Free(g_on);
    BaseBitmap::Free(g_off);
    BaseBitmap::Free(g_multiple);
}



