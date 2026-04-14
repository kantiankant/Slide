#ifndef CONFIG_H
#define CONFIG_H

#define MOD Mod4Mask
#define WIN_MOVE_STEP 40
#define PAN_STEP 60 

static const char *term[] = { "st", NULL };
static const char *menu[] = { "rofi", "-show", "drun", NULL };

static const char *vol_up[]    = { "sh", "-c", "wpctl set-volume @DEFAULT_AUDIO_SINK@ 1%+ && VOL=$(wpctl get-volume @DEFAULT_AUDIO_SINK@ | awk '{printf \"%d\", $2 * 100}') && notify-send -h string:x-dunst-stack-tag:osd -h int:value:$VOL -t 1500 \"Volume\" \"$VOL%\" && kill -USR1 $(pidof kantbar)", NULL };
static const char *vol_down[]  = { "sh", "-c", "wpctl set-volume @DEFAULT_AUDIO_SINK@ 1%- && VOL=$(wpctl get-volume @DEFAULT_AUDIO_SINK@ | awk '{printf \"%d\", $2 * 100}') && notify-send -h string:x-dunst-stack-tag:osd -h int:value:$VOL -t 1500 \"Volume\" \"$VOL%\" && kill -USR1 $(pidof kantbar)", NULL };
static const char *vol_mute[]  = { "sh", "-c", "wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle && notify-send -h string:x-dunst-stack-tag:osd -t 1500 \"Volume\" \"Muted\" && kill -USR1 $(pidof kantbar)", NULL };
static const char *bri_up[]    = { "sh", "-c", "brightnessctl set 1%+ && BRI=$(brightnessctl get) && MAX=$(brightnessctl max) && VAL=$((BRI * 100 / MAX)) && notify-send -h string:x-dunst-stack-tag:osd -h int:value:$VAL -t 1500 \"Brightness\" \"$VAL%\" && kill -USR1 $(pidof kantbar)", NULL };
static const char *bri_down[]  = { "sh", "-c", "brightnessctl set 1%- && BRI=$(brightnessctl get) && MAX=$(brightnessctl max) && VAL=$((BRI * 100 / MAX)) && notify-send -h string:x-dunst-stack-tag:osd -h int:value:$VAL -t 1500 \"Brightness\" \"$VAL%\" && kill -USR1 $(pidof kantbar)", NULL };
static const char *quit[]      = { "pkill", "slide", NULL };
const char* shot[]    = {"sh", "-c", "maim -s ~/Screenshots/$(date +%Y-%m-%d_%H-%M-%S).png", 0};
static const char *file[]      = { "wezterm", "-e", "yazi", NULL };
static const char *ani[]      = { "st", "-e", "ani-cli", NULL };
static const char *mus[]      = { "st", "-e", "cmus", NULL };






static key keys[] = {
    { MOD,             XK_q,                    run,        {.com = term}     },
    { MOD,             XK_e,                    run,        {.com = file}     },
    { MOD,             XK_r,                    run,        {.com = mus}      },
    { MOD|ShiftMask,   XK_r,    	        run,        {.com = ani}      },
    { MOD,             XK_space,                run,        {.com = menu}     },
    { MOD,             XK_w,                    win_kill,   {0}               },
    { MOD,             XK_c,                    win_center, {0}               },
    { MOD,             XK_f,                    win_fs,     {0}               },
    { MOD,             XK_h,                    pan_by_key, {.i = 0}          },
    { MOD,             XK_l,                    pan_by_key, {.i = 1}          },
    { MOD,             XK_k,                    pan_by_key, {.i = 2}          },
    { MOD,             XK_j,                    pan_by_key, {.i = 3}          },
    { MOD|ShiftMask,   XK_h,                    win_move,   {.i = 0}          },
    { MOD|ShiftMask,   XK_l,                    win_move,   {.i = 1}          },
    { MOD|ShiftMask,   XK_k,                    win_move,   {.i = 2}          },
    { MOD|ShiftMask,   XK_j,                    win_move,   {.i = 3}          },
    { MOD|ShiftMask,   XK_e,                    run,        {.com = quit}     },
    { MOD|ControlMask, XK_l,                    win_cycle,  {.i = 1}          },
    { MOD|ControlMask, XK_h,                    win_cycle,  {.i = 0}          },
    { 0,               XF86XK_AudioRaiseVolume, run,        {.com = vol_up}   },
    { 0,               XF86XK_AudioLowerVolume, run,        {.com = vol_down} },
    { 0,               XF86XK_AudioMute,        run,        {.com = vol_mute} },
    { 0,               XF86XK_MonBrightnessUp,  run,        {.com = bri_up}   },
    { 0,               XF86XK_MonBrightnessDown,run,        {.com = bri_down} },
    { 0,               XK_Print,                run,        {.com = shot}     },
};

#endif

