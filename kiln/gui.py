"""Optional Tkinter-backed desktop GUI and 2D graphics runtime for Kiln.

The module imports Tkinter lazily so terminal-only Kiln programs do not require
a graphical environment.
"""
from pathlib import Path
import sys


class GuiWindow:
    def __init__(self, root, interpreter):
        self.root = root
        self.interpreter = interpreter
        self._animations = []

    def __repr__(self):
        return "<Kiln window>"


class GuiCanvas:
    def __init__(self, widget):
        self.widget = widget
        self._images = []

    def __repr__(self):
        return "<Kiln canvas>"


class GuiInput:
    def __init__(self, widget):
        self.widget = widget

    def __repr__(self):
        return "<Kiln input>"


class GuiAnimation:
    def __init__(self, window, callback_id):
        self.window = window
        self.callback_id = callback_id

    def stop(self):
        try:
            self.window.root.after_cancel(self.callback_id)
        except Exception:
            pass


def _tk():
    try:
        import tkinter as tk
        return tk
    except ImportError as exc:
        raise RuntimeError(
            "Kiln GUI requires Tkinter. On Debian/Ubuntu/Linux Kiln, install it with: "
            "sudo apt install python3-tk"
        ) from exc


def _arity(name, args, minimum, maximum=None):
    maximum = minimum if maximum is None else maximum
    if not minimum <= len(args) <= maximum:
        if minimum == maximum:
            expected = str(minimum)
        else:
            expected = f"{minimum} to {maximum}"
        raise RuntimeError(f"{name}() expects {expected} argument(s), got {len(args)}")


def _window(value, function_name):
    if not isinstance(value, GuiWindow):
        raise RuntimeError(f"{function_name}() expected a window as its first argument")
    return value


def _canvas(value, function_name):
    if not isinstance(value, GuiCanvas):
        raise RuntimeError(f"{function_name}() expected a canvas as its first argument")
    return value


def _callback(interpreter, callback, function_name):
    from .interpreter import KilnFunction, BoundMethod
    if not isinstance(callback, (KilnFunction, BoundMethod)):
        raise RuntimeError(f"{function_name}() callback must be a Kiln function")
    return callback


def dispatch(interpreter, name, args):
    """Execute a public Kiln GUI/graphics built-in."""
    _arity(name, args, {
        "window": 3, "label": 4, "button": 5, "text_input": 4,
        "get_value": 1, "set_value": 2, "canvas": 5,
        "draw_rect": 6, "draw_circle": 5, "draw_line": 6,
        "draw_text": 5, "draw_image": 4, "clear_canvas": 1, "show": 1,
        "on_key": 2, "on_click": 2, "on_mouse_move": 2,
        "animate": 3, "stop_animation": 1,
        "close": 1, "set_title": 2,
    }[name])
    tk = _tk()

    try:
        if name == "window":
            title, width, height = args
            root = tk.Tk()
            root.title(str(title))
            root.geometry(f"{int(width)}x{int(height)}")
            root.minsize(160, 120)
            return GuiWindow(root, interpreter)

        if name == "label":
            win = _window(args[0], name)
            text, x, y = args[1:]
            widget = tk.Label(win.root, text=str(text))
            widget.place(x=int(x), y=int(y))
            return widget

        if name == "button":
            win = _window(args[0], name)
            text, x, y, callback = args[1:]
            callback = _callback(interpreter, callback, name)

            def invoke():
                try:
                    interpreter.call_function(callback, [])
                except Exception as exc:
                    print(f"Kiln GUI callback error: {exc}", file=sys.stderr)

            import sys
            widget = tk.Button(win.root, text=str(text), command=invoke)
            widget.place(x=int(x), y=int(y))
            return widget

        if name == "text_input":
            win = _window(args[0], name)
            x, y, width = args[1:]
            widget = tk.Entry(win.root, width=max(1, int(width)))
            widget.place(x=int(x), y=int(y))
            return GuiInput(widget)

        if name == "get_value":
            if not isinstance(args[0], GuiInput):
                raise RuntimeError("get_value() expects an input field")
            return args[0].widget.get()

        if name == "set_value":
            if not isinstance(args[0], GuiInput):
                raise RuntimeError("set_value() expects an input field")
            widget = args[0].widget
            widget.delete(0, "end")
            widget.insert(0, str(args[1]))
            return True

        if name == "canvas":
            win = _window(args[0], name)
            width, height, x, y = args[1:]
            widget = tk.Canvas(win.root, width=int(width), height=int(height),
                               background="white", highlightthickness=0)
            widget.place(x=int(x), y=int(y))
            return GuiCanvas(widget)

        if name == "draw_rect":
            cv = _canvas(args[0], name)
            x, y, width, height, color = args[1:]
            return cv.widget.create_rectangle(
                int(x), int(y), int(x) + int(width), int(y) + int(height),
                fill=str(color), outline=str(color)
            )

        if name == "draw_circle":
            cv = _canvas(args[0], name)
            x, y, radius, color = args[1:]
            radius = int(radius)
            return cv.widget.create_oval(
                int(x) - radius, int(y) - radius,
                int(x) + radius, int(y) + radius,
                fill=str(color), outline=str(color)
            )

        if name == "draw_line":
            cv = _canvas(args[0], name)
            x1, y1, x2, y2, color = args[1:]
            return cv.widget.create_line(int(x1), int(y1), int(x2), int(y2),
                                         fill=str(color), width=2)

        if name == "draw_text":
            cv = _canvas(args[0], name)
            text, x, y, color = args[1:]
            return cv.widget.create_text(int(x), int(y), text=str(text),
                                         fill=str(color), anchor="nw")

        if name == "clear_canvas":
            cv = _canvas(args[0], name)
            cv.widget.delete("all")
            cv._images.clear()
            return True

        if name == "draw_image":
            cv = _canvas(args[0], name)
            image_path, x, y = args[1:]
            path = Path(str(image_path))
            if not path.is_absolute():
                path = interpreter.source_dir / path
            path = path.resolve()
            if not path.is_file():
                raise RuntimeError(f"Image file not found: {path}")
            photo = tk.PhotoImage(file=str(path))
            cv._images.append(photo)  # Tkinter requires a live Python reference.
            return cv.widget.create_image(int(x), int(y), image=photo, anchor="nw")

        if name == "show":
            win = _window(args[0], name)
            win.root.mainloop()
            return None

        if name == "on_key":
            win = _window(args[0], name)
            callback = _callback(interpreter, args[1], name)

            def key_handler(event):
                try:
                    interpreter.call_function(callback, [event.keysym])
                except Exception as exc:
                    print(f"Kiln GUI key callback error: {exc}", file=sys.stderr)

            win.root.bind("<Key>", key_handler)
            win.root.focus_force()
            return True

        if name in ("on_click", "on_mouse_move"):
            cv = _canvas(args[0], name)
            callback = _callback(interpreter, args[1], name)
            sequence = "<Button-1>" if name == "on_click" else "<Motion>"

            def mouse_handler(event):
                try:
                    interpreter.call_function(callback, [event.x, event.y])
                except Exception as exc:
                    print(f"Kiln mouse callback error: {exc}", file=sys.stderr)

            import sys
            cv.widget.bind(sequence, mouse_handler)
            return True

        if name == "animate":
            win = _window(args[0], name)
            interval, callback = args[1:]
            interval = max(1, int(interval))
            callback = _callback(interpreter, callback, name)
            state = {"active": True}

            def tick():
                if not state["active"]:
                    return
                try:
                    interpreter.call_function(callback, [])
                except Exception as exc:
                    state["active"] = False
                    print(f"Kiln animation callback error: {exc}", file=sys.stderr)
                    return
                callback_id = win.root.after(interval, tick)
                animation.callback_id = callback_id

            callback_id = win.root.after(interval, tick)
            animation = GuiAnimation(win, callback_id)
            win._animations.append(animation)
            return animation

        if name == "stop_animation":
            animation = args[0]
            if not isinstance(animation, GuiAnimation):
                raise RuntimeError("stop_animation() expects an animation handle")
            animation.stop()
            return True

        if name == "close":
            win = _window(args[0], name)
            win.root.destroy()
            return True

        if name == "set_title":
            win = _window(args[0], name)
            win.root.title(str(args[1]))
            return True

    except RuntimeError:
        raise
    except Exception as exc:
        raise RuntimeError(f"{name}() failed: {exc}") from exc

    raise RuntimeError(f"Unknown Kiln GUI function: {name}")
