/**
 * Called from keyboard_init in keyboard.c
 */
function browser_init_keys(keystate, keylayout) {
    console.log('browser_init_keys', keystate, keylayout);
    getEmuInput().setupArculatorKeyboard(keystate, keylayout);
}

/**
 * Called from mouse_init in input_emscripten.c
 */
function browser_init_mouse(mxPtr, myPtr, buttonsPtr, mouseModePtr) { 
    console.log('browser_init_mouse', mxPtr, myPtr, mouseModePtr);
    getEmuInput().setupArculatorMouse(buttonsPtr, mxPtr, myPtr, mouseModePtr);
}

function notify_mouse_lock_required(needed) {
    console.log('notify_mouse_lock_required', needed);
    
    getEmuInput().mouseCaptureNeeded = needed;
    if (document.pointerLockElement) {
        if (!needed)
            document.exitPointerLock();
    }   
    console.log('toggle rel-mouse-needed', needed);
    document.body.classList.toggle('rel-mouse-needed', needed);
}

mergeInto(LibraryManager.library, {
    browser_init_keys: browser_init_keys,
    browser_init_mouse: browser_init_mouse,
    notify_mouse_lock_required: notify_mouse_lock_required
});


