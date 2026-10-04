# Real-Time Mode-Dependent WCET Immediate-Mode GUI Engine

A deterministic, static-memory, zero-allocation Immediate Mode Graphical User Interface (IMGUI) designed for embedded and real-time systems requiring **provable mode-dependent Worst-Case Execution Time (WCET)** bounds.

The **entire GUI library contains zero floating-point operations**, enabling native execution on low-power microcontrollers, automotive ECUs, avionics displays, and FPGA soft-cores (such as base integer RISC-V architectures) lacking a hardware Floating-Point Unit (FPU). The engine eliminates dynamic heap allocation, renders strictly on demand without non-stop polling loops, replaces floating popups with ergonomic single-plane view navigation, and bounds GPU synchronization deadlines.

---

## 1. System Architecture

```
+-----------------------------------------------------------------------------------+
|                              Application Logic                                    |
|         (Mode-Dependent State Machine: Schema, Data Grid, Hex/Text Viewer)        |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|                           Core GUI Engine (gui.c)                                 |
|  - 100% Integer & Fixed-Point Pipeline Across Entire Library (Zero Floats)        |
|  - Virtualized List & Table Architecture (gui_lst / gui_tbl)                      |
|  - Deterministic Two-Pass Traversal (GUI_INPUT -> GUI_RENDER)                     |
|  - Hierarchical 32-bit / 64-bit FNV-1a Widget Addressing                          |
|  - Single-Plane View Transitions (Zero Popups, Dropdowns, or Overlays)            |
+-----------------------------------------------------------------------------------+
                   |                                             |
                   v                                             v
+------------------------------------+         +------------------------------------+
|       Resource Engine (res.c)      |         |     Hardware Platform (sys.c/m)    |
| - L1 Hot Advance Array (256 B)     |         | - Event-Driven Blocking Poll       |
| - 8-Byte Packed Integer Quads      |         | - Demand-Driven Dirty Repainting   |
| - Loop-Bounded Run Fitting         |         | - Bounded OS Timing & Clipboards   |
+------------------------------------+         +------------------------------------+
                   \                                             /
                    \                                           /
                     v                                         v
+-----------------------------------------------------------------------------------+
|                          GPU Backend (gfx_mtl / gfx_vk)                           |
|  - Hard Real-Time GPU Synchronization Timeout (33 ms Deadline Cap)                |
|  - Static Storage Buffer Ring (GFX_BUF_SIZE = 512 KB, GFX_BUF_DEPTH = 2)          |
|  - Zero-Allocation Draw Submissions via Bindless Descriptors / Argument Buffers   |
+-----------------------------------------------------------------------------------+
```

---

## 2. Core Architectural Tenets

### 2.1 Provable Mode-Dependent WCET Bounds
Calculating a global, monolithic WCET bound across an entire interactive application yields figures too pessimistic for practical real-time scheduling. This architecture implements **Mode-Dependent WCET Analysis**:

* **Operational State Isolation**: The application partitions user interaction into discrete, mutually exclusive operational modes. Each mode executes a dedicated layout with strictly isolated code paths and bounded widget counts.
* **Static Loop Ceilings**: Dynamic loop conditions dependent on user data volume are eliminated. Every loop iterating over visible rows, table columns, text runs, layout slots, or search buffers is bound by compile-time constants.
* **No Unbounded Tree Traversals**: Because Immediate Mode re-evaluates the active interface functionally on every tick, there is no retained widget graph on the heap. This prevents unbounded recursive updates, style invalidations, or layout cascades.

---

### 2.2 Universal Hardware Portability via Zero Floating-Point Arithmetic
The **entire GUI library is implemented without floating-point math**. This design choice is driven by universal hardware portability rather than WCET alone:

* **Elimination of FPU Hardware Dependencies**: High-reliability embedded architectures, safety-critical microcontrollers, and soft-core processors often omit silicon for hardware FPUs to minimize die size, power draw, and verification complexity.
* **Removal of Soft-Float Overhead**: On hardware lacking an FPU, compilers link software floating-point emulation routines (`__aeabi_fadd`, `__subsf3`, etc.). These routines introduce substantial instruction overhead, variable branch latencies, and bloated binary footprints.
* **Pure Integer & Fixed-Point Pipeline**:
  - **Q16.16 Layout Solver**: Dynamic column proportions, flex weights, and stretch factors are calculated using integer arithmetic with Q16.16 fixed-point scaling ratios.
  - **L1-Resident Integer Font Metrics**: Glyph measurement uses an integer-only, 256-byte lookup table containing horizontal advances. String measurement is a linear sum of 8-bit unsigned integers.
  - **Packed 8-Byte Glyph Quads**: Rasterized glyph metrics (bearing offsets, texture atlas coordinates, and pixel dimensions) are packed into 8-byte integer records.
  - **Integer Geometry and Scaling**: Scissor clipping planes, coordinate offsets, DPI scaling, and color operations rely exclusively on integer division, bit shifts, and rounding biases.

---

### 2.3 Demand-Driven Rendering (No Non-Stop Rendering)
Continuous render loops executing at fixed refresh rates waste CPU cycles, saturate memory buses, drain power, and induce thermal throttling that degrades real-time guarantees. This system operates **strictly on demand**:

* **Event-Driven Sleeping**: When the interface state is quiescent and no animations or pending inputs exist, the platform thread yields execution to the OS scheduler using blocking calls (`XNextEvent` on Linux/X11, or an explicitly paused `MTKView` on macOS/Cocoa).
* **Dirty-Flag Dispatching**: The render pipeline triggers a frame tick only when hardware inputs arrive, window dimensions change, or application logic explicitly flags `needs_repaint`.
* **33 ms Bounded GPU Timeouts**: To prevent GPU command queue stalls or display server hangs from blocking CPU tasks, GPU synchronization fences enforce a hard 33 ms deadline. If a frame fails to retire within this limit, the frame is dropped gracefully to preserve the CPU scheduling timeline.

---

### 2.4 Strict Zero Runtime Allocations
Dynamic heap allocation (`malloc`, `free`, `realloc`) is prohibited during execution:

* **Pre-Mapped Memory Ceilings**: Vertex buffers (128 KB), index buffers (128 KB), and GPU-coherent shared memory rings (512 KB) are mapped once during initialization.
* **Bounded Working Pools**: All temporary string formatting, file paths, and clipboard exchanges operate within static, pre-allocated flat arrays.

---

## 3. The Heart of the GUI: Virtualized List and Table Architecture

The list (`gui_lst`) and table (`gui_tbl`) subsystems form the core of the engine. They provide virtualization, multi-column layout, keyboard navigation, and selection mechanics within fixed memory bounds and pure integer pipelines.

```
+-----------------------------------------------------------------------------------+
|                             gui_tbl (Table Subsystem)                             |
|  +-----------------------------------------------------------------------------+  |
|  |                   Header: gui_split + gui_tbl_hdr                           |  |
|  |  - Q16.16 Column Solver (Fixed / Weighted Dynamic Slots)                    |  |
|  |  - Interactive Splitter Separators (gui_sep) for runtime column resizing    |  |
|  |  - Header Slots with Integrated Sorting State (ASC / DESC) & Toggles        |  |
|  +-----------------------------------------------------------------------------+  |
|  |                   Body: gui_reg + gui_lst + gui_tbl_lst                     |  |
|  |  - Viewport Virtualization: Computes active visible index range [begin, end)  |  |
|  |  - Fixed Row-Column Stride: Eliminates pointer matrices / allocations       |  |
|  |  - Multi-Column Row Streaming: Slices each row into sub-column viewports    |  |
|  +-----------------------------------------------------------------------------+  |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|                              gui_lst (List Subsystem)                             |
|  +------------------------+ +------------------------+ +-----------------------+  |
|  |  gui_lst_lay (Layout)  | |  gui_lst_ctl (Control) | | gui_lst_sel (Selector)|  |
|  | - Horizontal / Vertical| | - Focus State Machine  | | - Single / Multi-Sel  |  |
|  | - Slot & Page Metrics  | | - Keyboard Cursor Nav  | | - FLEETING vs TOGGLE  |  |
|  | - Clamping & Centering | | - Page Up / Down Jumps | | - Shift / Ctrl Ranges |  |
|  +------------------------+ +------------------------+ +-----------------------+  |
+-----------------------------------------------------------------------------------+
```

### 3.1 List Subsystem (`gui_lst`)
The list engine decouples spatial layout, viewport culling, user input, and item selection into modular stages:

1. **Layout Engine (`gui_lst_lay`)**:
   - Manages linear item placement along either horizontal or vertical axes.
   - Computes layout metrics—such as total space, item slots (dimension plus gap), page size, and offset indices—using integer division and bitwise shifts.
   - Provides alignment calculations including centering, fitting to start/end, and clamping to bring target items into view.
2. **Viewport Virtualization (`gui_lst_view`)**:
   - Given a dataset of arbitrary size, it calculates the visible window:
     $$\text{begin} = \text{off\_idx}, \quad \text{end} = \min(\text{begin} + \text{visible\_count}, \text{total\_count})$$
   - Only on-screen items are evaluated during immediate-mode iterations. Rendering execution time is $\mathcal{O}(\text{visible})$ rather than $\mathcal{O}(\text{total})$, providing constant-time performance regardless of data volume.
3. **Interaction & Focus Control (`gui_lst_ctl`)**:
   - Maintains cursor positions and focus states.
   - Handles directional input (Arrow keys), page jumps (Page Up / Page Down), boundary jumps (Home / End), and activation (Enter / Space).
   - Draws a deterministic 1-pixel integer cursor boundary over the active item.
4. **Selection Engine (`gui_lst_sel`)**:
   - Supports single-selection (`GUI_LST_SEL_SINGLE`) and multi-selection (`GUI_LST_SEL_MULTI`).
   - Implements two selection behaviors:
     - `GUI_LST_SEL_BHV_FLEETING`: Standard desktop selection where clicking clears previous selections unless modified by Ctrl or Shift.
     - `GUI_LST_SEL_BHV_TOGGLE`: Independent item toggling suited for touchscreens or specialized control interfaces.
   - Computes contiguous selection ranges for Shift-click operations using integer min/max arithmetic.
5. **Bitset Filtering**:
   - Lists support hardware-accelerated bitset filtering using compiler intrinsics (`__builtin_ctzll` and `__builtin_popcountll`).
   - Filtered lists skip hidden items without copying or reallocating arrays, maintaining deterministic execution bounds.

---

### 3.2 Table Subsystem (`gui_tbl`)
Tables extend the list virtualization pipeline into two dimensions by combining interactive splitters with virtualized row streaming:

1. **Header Layout & Interactive Splitters (`gui_split` / `gui_tbl_hdr`)**:
   - Table columns are defined via layout slots that are either fixed pixel widths (`GUI_LAY_SLOT_FIX`) or dynamically weighted shares (`GUI_LAY_SLOT_DYN`).
   - The integer layout solver distributes available horizontal space across columns.
   - Interactive separator handles (`gui_sep`) allow columns to be resized at runtime with min/max constraints, clamping separator positions without memory allocation.
   - Headers manage sorting states (`GUI_SORT_ASC`, `GUI_SORT_DESC`) and column-level toggles (lock state, visibility, custom icons).
2. **Virtualized Row Streaming (`gui_tbl_lst`)**:
   - Vertical virtualization operates through `gui_lst`, ensuring only visible rows are submitted for rendering.
   - Horizontal scrolling links header and row cells synchronously via coordinate offsets.
   - Tables employ a dense strided cell structure (e.g., $128 \text{ rows} \times 8 \text{ columns}$). This eliminates pointer indirection and jagged memory layouts, keeping memory access localized.
3. **Column-Level Viewport Slicing (`gui_tbl_lst_elm_col`)**:
   - During row rendering, each cell is carved out of the row’s bounding box using integer cutting routines (`gui_cut_lhs`).
   - Scissor clipping is applied to each column slot, ensuring that text, badges, toggles, or icons never overflow into adjacent columns.
   - Standard cell types (Text, Text + Icon, Formatted Dates, Numeric Values, Interactive Checkboxes, and Action Links) are built into the column emission pipeline.

---

## 4. Design Philosophy: Dedicated Views Over Transient Overlays

This GUI eliminates **popups, dropdown menus, context menus, and floating modal dialogs**.

This is an intentional architectural decision based on interaction ergonomics, immediate-mode lifecycle characteristics, and real-time execution predictability:

```
 Traditional Desktop Paradigm                This Engine's Dedicated-View Paradigm
 (Stacked Overlays & Z-Fighting)               (Single-Plane State Machine)
 
 ┌─────────────────────────────┐              ┌─────────────────────────────┐
 │ Main View                   │              │ Main View                   │
 │   ┌───────────────────────┐ │              │                             │
 │   │ Select Field          │ │  Click Field │   [ Select Table... ]       │
 │   │ ┌───────────────────┐ │ │ ───────────► └──────────────┬──────────────┘
 │   │ │ Dropdown Overlay  │ │ │                             │ Transition
 │   │ │ (Z-Index Conflict,│ │ │                             ▼
 │   │ │  Edge Snapping,   │ │ │              ┌─────────────────────────────┐
 │   │ │  Modal Grabs)     │ │ │              │ Dedicated Selection View    │
 │   │ └───────────────────┘ │ │              │  - Item 1                   │
 │   └───────────────────────┘ │              │  - Item 2 (Full Virtual List│
 └─────────────────────────────┘              │  - Item 3   Zero Z-Layering)│
                                              └─────────────────────────────┘
```

### 4.1 The "Single-Plane" Usability Model
Floating window overlays introduce friction into user interfaces:
* They demand high pointer precision.
* They are prone to accidental dismissal via stray clicks outside their bounds.
* They create visual clutter when layered across small screens or high-density displays.

Adopting the design philosophy of early mobile interfaces (such as the original iPhone OS), this engine treats **every selection task as its own dedicated view**. When a user needs to pick an option, select an entity, or configure a filter, the interface transitions its active container into a full, dedicated view. 

This model minimizes cognitive load: the user is presented with a clear workspace to inspect items, make a choice, and return, without fighting transient popups.

### 4.2 The Immediate-Mode Advantage
In traditional retained-mode GUI toolkits, swapping an entire view to select an item is cumbersome: the application must instantiate new views, manage widget lifetimes, destroy old hierarchies, and handle complex navigation controllers. Retained-mode toolkits defaulted to popups because floating overlays felt like the path of least resistance.

In Immediate Mode, **rendering an entirely different view carries zero structural cost**. Because interface geometry is reconstructed functionally every frame:
* There are no overlay canvas allocations.
* There is no retained object lifecycle to construct, manage, or tear down.
* Transitioning from a data grid to a selection browser requires only updating a state variable (`stbl->state = TBL_VIEW_SELECT`). The previous view is simply not emitted on the next tick.

### 4.3 Determinism and WCET Elimination
From a real-time verification perspective, floating contextual overlays introduce architectural problems:
* **No Edge-Flipping or Boundary Math**: Popups require runtime collision detection against screen edges (e.g., determining whether a menu should open downward, flip upward, or shift sideways). Dedicated views adhere to static, fixed-point layout boundaries known at compile time.
* **Single-Plane Rendering (Zero Z-Sorting)**: Traditional popups require multi-pass rendering, depth sorting, and complex modal backdrops. A single-plane view guarantees that primitives are emitted in a single, predictable linear stream with simple scissor clipping.
* **No Modal Capture Loops**: Popups necessitate global pointer grabs, modal event loops, and priority filters to detect clicks outside the popup bounds. Eliminating popups keeps input processing strictly local, hierarchical, and single-pass.

---

## 5. Taming Dynamic Data: Mode-Dependent WCET in the SQLite Viewer

A relational database explorer presents an unpredictable workload for real-time systems: schemas are arbitrary, row counts range from zero to millions, column counts vary, and payloads can range from small integers to large BLOBs.

Standard GUI toolkits handle this using dynamic memory allocations, linked lists of row objects, and unbounded text measurement. In this project, the GUI acts as a **strict spatial and temporal governor** over SQLite, transforming an unbounded data source into a set of predictable, mode-dependent execution envelopes.

```
                              Arbitrary SQLite Database
                      (Millions of Rows, Unbounded Text / Blobs)
                                          │
                                          ▼
                   ┌──────────────────────────────────────────────┐
                   │        GUI Spatial & Temporal Governor       │
                   │  - Dual-Axis Window Virtualization (128 x 8) │
                   │  - Fixed Strided Flat Cache Memory           │
                   │  - Payload Truncation & Mode Escalation      │
                   └──────────────────────┬───────────────────────┘
                                          │
                  ┌───────────────────────┴───────────────────────┐
                  ▼                                               ▼
       ┌─────────────────────┐                         ┌─────────────────────┐
       │     Schema Mode     │                         │    Grid Data Mode   │
       │ (TBL_VIEW_SELECT)   │                         │(DB_TBL_VIEW_DSP_DATA│
       │ Bound: <= 128 Items │                         │ Bound: <= 128x8     │
       └─────────────────────┘                         └──────────┬──────────┘
                                                                  │
                                   Large Payload / Blob Detected  │
                                   (Transition to Sub-Mode)       │
                                                                  ▼
                                                       ┌─────────────────────┐
                                                       │   Hex / Text Mode   │
                                                       │  Bound: <= 128 Rows │
                                                       └─────────────────────┘
```

### 5.1 Dual-Axis Bounded Window Virtualization
Regardless of whether a database table contains 10 rows or 10,000,000 rows, the GUI never permits more than a fixed upper bound of data to enter its layout and rendering pipeline:

* **Vertical Window Ceiling**: The table viewport queries and caches at most `DB_MAX_TBL_ROWS` (128 rows) via parameterized `LIMIT ?, ?` queries. Scrolling shifts the offset index, but the number of row iterations per frame tick remains fixed.
* **Horizontal Column Window**: Relational tables can contain arbitrary numbers of columns. The table viewport limits horizontal display to a sliding sub-window of at most `DB_MAX_TBL_ROW_COLS` (8 columns). Horizontal scrolling shifts column pointers within a fixed 8-slot array (`stbl->row.cols.lo` to `stbl->row.cols.hi`).
* **Strided Flat Cell Memory**: Instead of dynamic pointer matrices (`char***`), the viewer stores cells in a flat static array with a fixed stride of 8 ($128 \times 8 = 1024$ entries). Inactive columns within a row are padded with empty static records, maintaining consistent memory offsets and eliminating heap fragmentation.

### 5.2 Payload Truncation and Mode Escalation
Unbounded strings and BLOBs violate static WCET analysis because measuring and rasterizing variable character sequences consumes unpredictable CPU time. The GUI resolves this through length limits and view transitions:

* **Preview Truncation**: During grid row rendering, text columns are capped to a maximum byte length (`DB_MAX_FLTR_ELM_STR` = 64 bytes). Longer strings are clamped at the database query boundary via SQLite's `SUBSTR(..., 1, 1024)`.
* **Discrete Mode Escalation**: When a cell contains a binary BLOB or multi-line text, the grid renders a lightweight action link (`[Open]`). Activating this widget transitions the viewer out of the grid mode into an isolated sub-mode (`DB_TBL_VIEW_DSP_DATA_BLOB` or `DB_TBL_VIEW_DSP_DATA_STR`).
* **Bounded Sub-Modes**:
  - **Hex Inspection Sub-Mode**: The raw binary data is read in fixed-size chunks (`DB_SQL_IO_BUF_SIZ` = 64 KB). The display formats at most `DB_MAX_BLB_ROW_CNT` (128 rows), with each row representing exactly 16 bytes formatted into hex and ASCII pairs.
  - **Paged Text Sub-Mode**: Multi-line strings stream line-by-line into a fixed array of at most 128 row offsets, bounding the line measurement and rendering loops.

### 5.3 Discrete Operational Timing Envelopes
By partitioning user interaction into distinct modes, the viewer guarantees that every frame tick belongs to an isolated WCET profile:

| Viewer Mode | Active Bounds | WCET Characteristics |
| :--- | :--- | :--- |
| **Schema Introspection** (`TBL_VIEW_SELECT`) | Max 128 schema entities | Fixed-cost scan over master table; iterates a single-column list. |
| **Grid Data Viewer** (`DB_TBL_VIEW_DSP_DATA`) | Max $128 \times 8$ cells | Constant-time execution relative to table size; linear accumulation of visible cell widths. |
| **BLOB Hex Inspector** (`DB_TBL_BLB_HEX`) | Max 128 rows $\times$ 16 bytes | Bounded byte formatting into static string buffers; constant glyph run length per line. |
| **Large Text Viewer** (`DB_TBL_VIEW_DSP_DATA_STR`) | Max 128 text lines | Paged line accumulation; strictly limited string bounds per visible slice. |
| **Column Configuration** (`DB_TBL_VIEW_DSP_LAYOUT`) | Max 128 column definitions | Iterates column metadata list; toggles lock state and column selection bitsets. |
| **Filter Expression Builder** (`DB_TBL_VIEW_DSP_FILTER`) | Max 8 active filter slots | Fixed 8-slot iteration evaluating filter match strings against column indices. |

By enforcing these static ceilings, the GUI decouples its execution time from the underlying database's data volume, proving that an interactive tool can maintain strict real-time guarantees over complex dynamic workloads.

---

## 6. Compile-Time Limits & WCET Verification Matrix

All runtime data structures are statically sized at compile time to satisfy real-time determinism:

| Constant | Value | WCET & Architectural Impact |
| :--- | :--- | :--- |
| `GUI_MAX_VIEWS` | 16 | Limits active view containers and viewport clipping stack depth. |
| `GUI_MAX_COLS` | 8 | Fixes column layout calculations to $\mathcal{O}(1)$ static steps. |
| `GUI_MAX_VISIBLE_ROWS` | 64 | Bounds the maximum number of row iterations per frame. |
| `GUI_MAX_CELLS` | 512 | Caps total visible interactive cell evaluations ($64 \times 8$). |
| `DB_MAX_TBL_ROWS` | 128 | Upper bound on rows processed during a single grid query and render tick. |
| `DB_MAX_TBL_ROW_COLS` | 8 | Fixed column stride for multi-column row streaming. |
| `DB_MAX_BLB_ROW_CNT` | 128 | Hard bound on hex viewer rows (16 bytes per row). |
| `RES_GLYPH_SLOTS` | 256 | Direct table lookup for ASCII/Latin glyphs; eliminates tree searches. |
| `RES_FNT_MAX_RUN` | 16 | Bounds text run inner loops to a constant 16 iterations. |
| `CFG_GUI_VTX_MEMORY` | 128 KB | Static memory ceiling for 2D primitives (`struct gfx_prim`). |
| `CFG_GUI_IDX_MEMORY` | 128 KB | Static memory ceiling for primitive indexing. |
| `GFX_VK_BUF_DEPTH` | 2 | Double-buffered GPU command/staging ring; prevents drift. |
| `GFX_VK_GPU_TIMEOUT_MS`| 33 ms | Hard real-time limit on fence waits; prevents CPU schedule overruns. |

## Screenshots
<img width="803" alt="image1" src="https://github.com/user-attachments/assets/51184fc2-e03c-4cb6-beeb-a6fc0b8ab562" />
<img width="801" alt="image2" src="https://github.com/user-attachments/assets/bcf8075b-4fed-4c8d-b9dd-211db28998ed" />
<img width="801" alt="image5" src="https://github.com/user-attachments/assets/bdc3d5d3-d39b-40bc-94dc-4367090bddbb" />
<img width="801" alt="image3" src="https://github.com/user-attachments/assets/07817d24-df77-4c8d-99c2-8993101e928c" />
<img width="800" alt="image4" src="https://github.com/user-attachments/assets/5577b81e-1574-408f-81fe-ca2e37a235a6" />

