 #ifdef KILN_WITH_RAYLIB
static int kiln_key_code(const char *name) {
    if (!strcmp(name,"escape")) return KEY_ESCAPE;
    if (!strcmp(name,"space")) return KEY_SPACE;
    if (!strcmp(name,"enter")) return KEY_ENTER;
    if (!strcmp(name,"tab")) return KEY_TAB;
    if (!strcmp(name,"left")) return KEY_LEFT;
    if (!strcmp(name,"right")) return KEY_RIGHT;
    if (!strcmp(name,"up")) return KEY_UP;
    if (!strcmp(name,"down")) return KEY_DOWN;
    if (!strcmp(name,"backspace")) return KEY_BACKSPACE;
    if (strlen(name)==1 && isalpha((unsigned char)name[0]))
        return toupper((unsigned char)name[0]);
    if (strlen(name)>=2 && name[0]=='f') {
        int n=atoi(name+1);
        if(n>=1 && n<=12) return KEY_F1+n-1;
    }
    return -1;
}
static int kiln_mouse_code(const char *name) {
    if (!strcmp(name,"left")) return MOUSE_BUTTON_LEFT;
    if (!strcmp(name,"right")) return MOUSE_BUTTON_RIGHT;
    if (!strcmp(name,"middle")) return MOUSE_BUTTON_MIDDLE;
    return -1;
}
static Color kiln_color(Value *a) {
    return (Color){(unsigned char)a[0].n,(unsigned char)a[1].n,
                   (unsigned char)a[2].n,255};
}
#endif
