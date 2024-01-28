- Introduction
  - For tool immmediate mode ui best to divide code up into three parts: core, widget and utility code
    - core makes up the reusable portion with some basic state setup like unique id generation
    - utility code for layouting, input, tooltips, mouse cursor state,...
    - widget code with specific input handling and rendering code like buttons, scrollbars, labels
  - Similar to retain mode ui we want the divide for composability and prevent coupling to much to internal core state
- Core
  - All Ui is visual hierarchy of rectangles.
  - Even widgets are composed like that
    - buttons are made out of a general purpose button background and another visual widget like labels, icons or combination
    - scrollbars are made out of other rectangles with two buttons and the actual scroll element in the center
  - Therefore smallest element in the core is a rectangle or `panel` which is how I will call it from now on
  - What it looks like in code:
```c
enum gui_state {
  GUI_HIDDEN,
  GUI_NORMAL,
  GUI_FOCUSED,
  GUI_DISABLED,
};
struct gui_panel {
  struct gui_box box;
  unsigned long long id;
  enum gui_state state;
  int max[2];
  unsigned is_hot : 1;
};
```
  - very basic with `gui_box` being the actual rectangle, id an unique identifier, state for some basic hints for updating widgets and finally max for maximum extend of all child panels
  - Since I don't store any permanent state `panel` live on the stack and are filled up each frame idientified by generated Id
  - Hierarchy is mapped in code by nested `begin` and `end` functions calls working on these panels and later widgets.
    - `begin` calls will always do state setup and for widgets drawing
    - `end` calls for `panels` will update the parent hierarchy total area of children and for widgets will need to handle input

```c
struct gui_panel a = {.box = ...};
gui_panel_begin(ctx, &a, &ctx.root);
{
  struct gui_panel b = {.box = ...};
  gui_panel_begin(&ctx, &b, &a);
  {

  }
  gui_panel_end(&ctx, &b, &a);

  struct gui_panel c = {.box = ...};
  gui_panel_begin(&ctx, &c, &a);
  {

  }
  gui_panel_end(&ctx, &c, &a);

  struct gui_panel d = {.box = ...};
  gui_panel_begin(&ctx, &d, &a);
  {

  }
  gui_panel_end(&ctx, &d, &a);
}
gui_panel_end(ctx, &a, &ctx.root);
```

will generate following hierarchy:

```
          root
           |
           a
         / | \
         b c d

```
  - this for example could be our scrollbar with `b` and `d` being the scrollbar buttons, `c` the scroll element and `a` the scrollbar
```
    [<][--------[     ]---][>]
     b          c           d
```

  - Lets take a look at the `panel_begin` call and what it does.

```c
static void
gui_panel_begin(struct gui_ctx *ctx, struct gui_panel *p,
               struct gui_panel *parent, unsigned long long id) {
  p->id = parent ? fnv1au64(id, parent->id) : id;
  p->max[0] = p->max[1] = 0;

  const struct sys *s = ctx->sys;
  int in_box = gui_inbox(s->mouse.pos[0], s->mouse.pos[1], p);
  p->is_hov = !parent || (parent->is_hov && in_box);
  if ((parent && !parent->is_hot) || !in_box) {
    return;
  }
  p->is_hot = 1;
  ctx->hot = p->is_hot ? p->id : ctx->hot;
}
```
  - Once again a small function with three key insights.
    - The global state `gui_ctx` for the core only keeps track of the id of currently hot panel
    - Final `panel id` is generated based on passed id (in practise just a growing integer) and the parent id
      - Very important to create avalanches when parent changes then all children ids need to change as well!
    - The hotness (i.e. mouse over state) depends on the mouse position and if the parent is hot as well
      - Important for scroll region which can have children outside parent area we don't want to be able to handle input for
  - Now lets look at the `panel_end` function:

```c
static void
gui_panel_end(struct gui_ctx *ctx, struct gui_panel *pan,
                struct gui_panel *parent) {
  if (parent) {
    parent->max[0] = max(parent->max[0], pan->box.x.max);
    parent->max[1] = max(parent->max[1], pan->box.y.max);
  }
}
```
   - Simply just updates the `max` extends of the parent panel based on the panel max values
   - This is all for the core functions for our `panels`.

 - Utility

