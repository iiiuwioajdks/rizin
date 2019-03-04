/* radare - LGPL - Copyright 2009-2019 - nibble, pancake, dso */

#include "r_core.h"
#include "r_cons.h"

#define HASRETRY 1
#define HAVE_LOCALS 1
#define DEFAULT_NARGS 4

#define COLOR(ds, field) ((ds)->show_color ? (ds)->field : "")
#define COLOR_ARG(ds, field) ((ds)->show_color && (ds)->show_color_args ? (ds)->field : "")
#define COLOR_CONST(ds, color) ((ds)->show_color ? Color_ ## color : "")
#define COLOR_RESET(ds) COLOR_CONST(ds, RESET)

// ugly globals but meh
static ut64 emustack_min = 0LL;
static ut64 emustack_max = 0LL;

static const char* r_vline_a[] = {
	"|",  // LINE_VERT
	"|-", // LINE_CROSS
	"-",  // LINE_HORIZ
	":",  // LINE_UP
	",",  // LUP_CORNER
	"\\", // RDWN_CORNER
	"/",  // RUP_CORNER
	"`",  // LDWN_CORNER
	"->", // ARROW_RIGHT
	"=<", // ARROW_LEFT
};

static const char* r_vline_u[] = {
	"│", // LINE_VERT
	"├", // LINE_CROSS
	"─", // LINE_HORIZ
	"╎", // LINE_UP
	// "↑", // LINE_UP
	//"┌", // LUP_CORNER
	"┘", // LUP_CORNER
	"└", // RDWN_CORNER
	"┌", // RUP_CORNER
	"┐", // LDWN_CORNER
	">", // ARROW_RIGHT
	"<", // ARROW_LEFT
};

static const char* r_vline_uc[] = {
	"│", // LINE_VERT
	"├", // LINE_CROSS
	"─", // LINE_HORIZ
	// "↑", // LINE_UP
	"╎", // LINE_UP
	// "≀", // LINE_UP
	//"┌", // LUP_CORNER
	"╯", // LUP_CORNER
	"╰", // RDWN_CORNER
	"╭", // RUP_CORNER
	"╮", // LDWN_CORNER
	">", // ARROW_RIGHT
	"<", // ARROW_LEFT
};


#define DS_PRE_NONE         0
#define DS_PRE_EMPTY        1
#define DS_PRE_FCN_HEAD     2
#define DS_PRE_FCN_MIDDLE   3
#define DS_PRE_FCN_TAIL     4

// TODO: what about using bit shifting and enum for keys? see libr/util/bitmap.c
// the problem of this is that the fields will be more opaque to bindings, but we will earn some bits
typedef struct {
	RCore *core;
	char str[1024], strsub[1024];
	bool immtrim;
	bool immstr;
	bool use_esil;
	bool show_color;
	bool show_color_bytes;
	bool show_color_args;
	int colorop;
	int acase;
	bool capitalize;
	bool show_flgoff;
	bool hasMidflag;
	bool hasMidbb;
	int atabs;
	int atabsonce;
	int atabsoff;
	int decode;
	bool pseudo;
	int filter;
	int interactive;
	bool jmpsub;
	bool varsub;
	bool show_lines;
	bool show_lines_bb;
	bool show_lines_ret;
	bool show_lines_call;
	bool show_lines_fcn;
	int linesright;
	int tracespace;
	int cyclespace;
	int cmtfold;
	int show_indent;
	bool show_dwarf;
	bool show_size;
	bool show_trace;
	bool show_family;
	bool asm_describe;
	int linesout;
	int adistrick;
	bool asm_meta;
	int asm_demangle;
	bool asm_instr;
	bool show_offset;
	bool show_offdec; // dupe for r_print->flags
	bool show_bbline;
	bool show_emu;
	bool pre_emu;
	bool show_emu_str;
	bool show_emu_stroff;
	bool show_emu_strinv;
	bool show_emu_strflag;
	bool show_emu_stack;
	bool show_emu_write;
	bool show_emu_strlea;
	bool show_emu_ssa;
	bool show_section;
	int show_section_col;
	bool show_section_perm;
	bool show_section_name;
	bool show_symbols;
	int show_symbols_col;
	bool show_offseg;
	bool show_flags;
	bool bblined;
	bool show_bytes;
	bool show_reloff;
	bool show_reloff_flags;
	bool show_comments;
	bool show_usercomments;
	bool asm_hints;
	bool asm_hint_jmp;
	bool asm_hint_cdiv;
	bool asm_hint_lea;
	int  asm_hint_pos;
	bool show_slow;
	Sdb *ssa;
	int cmtcol;
	bool show_calls;
	bool show_cmtflgrefs;
	bool show_cycles;
	bool show_stackptr;
	int stackFd;
	bool show_xrefs;
	bool show_cmtrefs;
	const char *show_cmtoff;
	bool show_functions;
	bool show_marks;
	bool show_asciidot;
	RStrEnc strenc;
	int cursor;
	int show_comment_right_default;
	RSpace *flagspace_ports;
	int show_flag_in_bytes;
	int lbytes;
	int show_comment_right;
	int pre;
	char *ocomment;
	int linesopts;
	int lastfail;
	int ocols;
	int lcols;
	int nb, nbytes;
	int show_utf8;
	int lines;
	int oplen;
	bool show_varaccess;
	bool show_vars;
	bool hinted_line;
	int show_varsum;
	int midflags;
	bool midbb;
	bool midcursor;
	bool show_noisy_comments;
	ut64 asm_highlight;
	const char *pal_comment;
	const char *color_comment;
	const char *color_usrcmt;
	const char *color_fname;
	const char *color_floc;
	const char *color_fline;
	const char *color_flow;
	const char *color_flow2;
	const char *color_flag;
	const char *color_label;
	const char *color_other;
	const char *color_nop;
	const char *color_bin;
	const char *color_math;
	const char *color_btext;
	const char *color_jmp;
	const char *color_cjmp;
	const char *color_call;
	const char *color_cmp;
	const char *color_swi;
	const char *color_trap;
	const char *color_ret;
	const char *color_push;
	const char *color_pop;
	const char *color_reg;
	const char *color_num;
	const char *color_mov;
	const char *color_invalid;
	const char *color_gui_cflow;
	const char *color_gui_dataoffset;
	const char *color_gui_background;
	const char *color_gui_alt_background;
	const char *color_gui_border;
	const char *color_linehl;
	const char *color_func_var;
	const char *color_func_var_type;
	const char *color_func_var_addr;

	RFlagItem *lastflag;
	RAnalHint *hint;
	RPrint *print;

	ut64 esil_old_pc;
	ut8* esil_regstate;
	bool esil_likely;

	int l;
	int middle;
	int indent_level;
	int indent_space;
	char *line;
	char *refline, *refline2;
	char *comment;
	char *opstr;
	char *osl, *sl;
	int stackptr, ostackptr;
	int index;
	ut64 at, vat, addr, dest;
	int tries, cbytes, idx;
	char chref;
	bool retry;
	bool mi_found;
	RAsmOp asmop;
	RAnalOp analop;
	RAnalFunction *fcn;
	RAnalFunction *pdf;
	const ut8 *buf;
	int len;
	int maxrefs;
	int foldxrefs;
	char *prev_ins;
	bool prev_ins_eq;
	int prev_ins_count;
	bool show_nodup;
	bool has_description;
	// caches
	char *_tabsbuf;
	int _tabsoff;
	bool dwarfFile;
	bool dwarfAbspath;
	bool showpayloads;
	bool showrelocs;
	int cmtcount;
	bool asm_anal;
	ut64 printed_str_addr;
	ut64 printed_flag_addr;
	ut64 min_ref_addr;

	PJ *pj; // not null iff printing json
	int buf_line_begin;
	const char *strip;
	int maxflags;
	int asm_types;
} RDisasmState;

static void ds_setup_print_pre(RDisasmState *ds, bool tail, bool middle);
static void ds_setup_pre(RDisasmState *ds, bool tail, bool middle);
static void ds_print_pre(RDisasmState *ds);
static void ds_pre_line(RDisasmState *ds);
static void ds_begin_line(RDisasmState *ds);
static void ds_newline(RDisasmState *ds);
static void ds_print_esil_anal(RDisasmState *ds);
static void ds_reflines_init(RDisasmState *ds);
static void ds_align_comment(RDisasmState *ds);
static RDisasmState * ds_init(RCore * core);
static void ds_build_op_str(RDisasmState *ds, bool print_color);
static void ds_pre_xrefs(RDisasmState *ds, bool no_fcnlines);
static void ds_show_xrefs(RDisasmState *ds);
static void ds_atabs_option(RDisasmState *ds);
static void ds_show_functions(RDisasmState *ds);
static void ds_show_comments_right(RDisasmState *ds);
static void ds_show_flags(RDisasmState *ds);
static void ds_update_ref_lines(RDisasmState *ds);
static int ds_disassemble(RDisasmState *ds, ut8 *buf, int len);
static void ds_control_flow_comments(RDisasmState *ds);
static void ds_print_lines_right(RDisasmState *ds);
static void ds_print_lines_left(RDisasmState *ds);
static void ds_print_cycles(RDisasmState *ds);
static void ds_print_family(RDisasmState *ds);
static void ds_print_stackptr(RDisasmState *ds);
static void ds_print_offset(RDisasmState *ds);
static void ds_print_op_size(RDisasmState *ds);
static void ds_print_trace(RDisasmState *ds);
static void ds_adistrick_comments(RDisasmState *ds);
static int ds_print_meta_infos(RDisasmState *ds, ut8* buf, int len, int idx );
static void ds_print_opstr(RDisasmState *ds);
static void ds_print_color_reset(RDisasmState *ds);
static int ds_print_middle(RDisasmState *ds, int ret);
static bool ds_print_labels(RDisasmState *ds, RAnalFunction *f);
static void ds_print_sysregs(RDisasmState *ds);
static void ds_print_fcn_name(RDisasmState *ds);
static void ds_print_as_string(RDisasmState *ds);
static void ds_print_core_vmode(RDisasmState *ds, int pos);
static void ds_print_dwarf(RDisasmState *ds);
static void ds_print_asmop_payload(RDisasmState *ds, const ut8 *buf);
static char *ds_esc_str(RDisasmState *ds, const char *str, int len, const char **prefix_out, bool is_comment);
static void ds_print_comments_right(RDisasmState *ds);
static void ds_print_ptr(RDisasmState *ds, int len, int idx);
static void ds_print_str(RDisasmState *ds, const char *str, int len, ut64 refaddr);
static char *ds_sub_jumps(RDisasmState *ds, char *str);
static void ds_start_line_highlight(RDisasmState *ds);
static void ds_end_line_highlight(RDisasmState *ds);
static bool line_highlighted(RDisasmState *ds);

R_API ut64 r_core_pava (RCore *core, ut64 addr) {
	if (core->pava) {
		RIOMap *map = r_io_map_get_paddr (core->io, addr);
		if (map) {
			return addr - map->delta + map->itv.addr;
		}
	}
	return addr;
}

static RAnalFunction *fcnIn(RDisasmState *ds, ut64 at, int type) {
	if (ds->fcn && r_tinyrange_in (&ds->fcn->bbr, at)) {
		return ds->fcn;
	}
	return r_anal_get_fcn_in (ds->core->anal, at, type);
}

static void get_bits_comment(RCore *core, RAnalFunction *f, char *cmt, int cmt_size) {
	if (core && f && cmt && cmt_size > 0 && f->bits && f->bits != core->assembler->bits) {
		const char *asm_arch = r_config_get (core->config, "asm.arch");
		if (asm_arch && *asm_arch && strstr (asm_arch, "arm")) {
			switch (f->bits) {
			case 16: strcpy (cmt, " (thumb)"); break;
			case 32: strcpy (cmt, " (arm)"); break;
			case 64: strcpy (cmt, " (aarch64)"); break;
			}
		} else {
			snprintf (cmt, cmt_size, " (%d bits)", f->bits);
		}
	} else {
		if (cmt) {
			cmt[0] = 0;
		}
	}
}

static const char * get_section_name(RCore *core, ut64 addr) {
	static char section[128] = "";
	static ut64 oaddr = UT64_MAX;
	if (oaddr == addr) {
		return section;
	}
	RBinObject *bo = r_bin_cur_object (core->bin);
	RBinSection *s = bo? r_bin_get_section_at (bo, addr, core->io->va): NULL;
	if (s) {
		snprintf (section, sizeof (section) - 1, "%10s ", s->name);
	} else {
		RListIter *iter;
		RDebugMap *map;
		*section = 0;
		r_list_foreach (core->dbg->maps, iter, map) {
			if (addr >= map->addr && addr < map->addr_end) {
				const char *mn = r_str_lchr (map->name, '/');
				if (mn) {
					strncpy (section, mn + 1, sizeof (section) - 1);
				} else {
					strncpy (section, map->name, sizeof (section) - 1);
				}
				break;
			}
		}
	}
	oaddr = addr;
	return section;
}

// up means if this lines go up, it controls whether to insert `_
// nl if we have to insert new line, it controls whether to insert \n
static void _ds_comment_align_(RDisasmState *ds, bool up, bool nl) {
	ds->cmtcount ++;
	if (ds->show_comment_right) {
		if (ds->show_color) {
			r_cons_printf (ds->pal_comment);
		}
		return;
	}
	//XXX fix this generate many dupes with section name
	const char *sn = ds->show_section ? get_section_name (ds->core, ds->at) : "";
	// if (ds->cmtcount == 0) {
		ds_align_comment (ds);
	// }
	sn = ds->show_section ? get_section_name (ds->core, ds->at) : "";
	ds_align_comment (ds);
	r_cons_print (COLOR_RESET (ds));
	ds_print_pre (ds);
	r_cons_printf ("%s%s%s%s%s  %s %s", nl? "\n": "", sn,
			COLOR (ds, color_flow),ds->refline, COLOR_RESET (ds),
			up? "": ".-", COLOR (ds, color_comment));
#if 0
// r_cons_printf ("(%d)", ds->cmtcount);
	if (ds->cmtcount == 1) {
		ds_align_comment (ds);
		r_cons_printf ("%s%s%s%s%s%s%s  %s %s", nl? "\n": "",
			COLOR_RESET (ds), COLOR (ds, color_fline),
			ds->pre, sn, ds->refline, COLOR_RESET (ds),
			up? "": ".v", COLOR (ds, color_comment));
	} else {
		r_cons_printf ("%s%s", COLOR (ds, color_comment), " "); //nl? "\n": "");
	}
#if 0
	if (!up || ds->cmtcount > 1) {
		r_cons_printf ("%s%s", COLOR (ds, color_comment), nl? "\n": "");
	} else {
		ds_align_comment (ds);
		r_cons_printf ("%s%s%s%s%s%s%s  %s %s", nl? "\n": "",
			COLOR_RESET (ds), COLOR (ds, color_fline),
			ds->pre, sn, ds->refline, COLOR_RESET (ds),
			up? "": "`-", COLOR (ds, color_comment));
	}
#endif
#endif
}
#define _ALIGN _ds_comment_align_ (ds, true, false)

static void ds_comment_lineup(RDisasmState *ds) {
	_ALIGN;
}

static void ds_comment_(RDisasmState *ds, bool align, bool nl, const char *format, va_list ap) {
	if (ds->show_comments) {
		if (ds->show_comment_right && align) {
			ds_align_comment (ds);
		} else {
			r_cons_printf ("%s", COLOR (ds, color_comment));
		}
	}

	r_cons_printf_list (format, ap);

	if (!ds->show_comment_right && nl) {
		ds_newline (ds);
	}
}

static void ds_comment(RDisasmState *ds, bool align, const char *format, ...) {
	va_list ap;
	va_start (ap, format);
	ds_comment_ (ds, align, align, format, ap);
	va_end (ap);
}

#define DS_COMMENT_FUNC(name, align, nl) \
	static void ds_comment_##name(RDisasmState *ds, const char *format, ...) { \
		va_list ap; \
		va_start (ap, format); \
		ds_comment_ (ds, align, nl, format, ap); \
		va_end (ap); \
	}

DS_COMMENT_FUNC (start, true, false)
DS_COMMENT_FUNC (middle, false, false)
DS_COMMENT_FUNC (end, false, true)

static void ds_comment_esil(RDisasmState *ds, bool up, bool end, const char *format, ...) {
	va_list ap;
	va_start (ap, format);
	if (ds->show_comments && up) {
		ds->show_comment_right ? ds_align_comment (ds) : ds_comment_lineup (ds);
	}
	r_cons_printf_list (format, ap);
	va_end (ap);

	if (ds->show_comments && !ds->show_comment_right) {
		if (end) {
			ds_newline (ds);
		}
	}
}

static void ds_print_esil_anal_fini(RDisasmState *ds) {
	RCore *core = ds->core;
	if (ds->show_emu && ds->esil_regstate) {
		RCore* core = ds->core;
		core->anal->last_disasm_reg = r_reg_arena_peek (core->anal->reg);
		const char *pc = r_reg_get_name (core->anal->reg, R_REG_NAME_PC);
		r_reg_arena_poke (core->anal->reg, ds->esil_regstate);
		r_reg_setv (core->anal->reg, pc, ds->esil_old_pc);
		R_FREE (ds->esil_regstate);
	}
	if (core && core->anal && core->anal->esil) {
		// make sure to remove reference to ds to avoid UAF
		core->anal->esil->user = NULL;
	}
}

static RDisasmState * ds_init(RCore *core) {
	RDisasmState *ds = R_NEW0 (RDisasmState);
	if (!ds) {
		return NULL;
	}
	ds->core = core;
	ds->strip = r_config_get (core->config, "asm.strip");
	ds->pal_comment = core->cons->context->pal.comment;
	#define P(x) (core->cons && core->cons->context->pal.x)? core->cons->context->pal.x
	ds->color_comment = P(comment): Color_CYAN;
	ds->color_usrcmt = P(usercomment): Color_CYAN;
	ds->color_fname = P(fname): Color_RED;
	ds->color_floc = P(floc): Color_MAGENTA;
	ds->color_fline = P(fline): Color_CYAN;
	ds->color_flow = P(flow): Color_CYAN;
	ds->color_flow2 = P(flow2): Color_CYAN;
	ds->color_flag = P(flag): Color_CYAN;
	ds->color_label = P(label): Color_CYAN;
	ds->color_other = P(other): Color_WHITE;
	ds->color_nop = P(nop): Color_BLUE;
	ds->color_bin = P(bin): Color_YELLOW;
	ds->color_math = P(math): Color_YELLOW;
	ds->color_btext = P(btext): Color_YELLOW;
	ds->color_jmp = P(jmp): Color_GREEN;
	ds->color_cjmp = P(cjmp): Color_GREEN;
	ds->color_call = P(call): Color_BGREEN;
	ds->color_cmp = P(cmp): Color_MAGENTA;
	ds->color_swi = P(swi): Color_MAGENTA;
	ds->color_trap = P(trap): Color_BRED;
	ds->color_ret = P(ret): Color_RED;
	ds->color_push = P(push): Color_YELLOW;
	ds->color_pop = P(pop): Color_BYELLOW;
	ds->color_reg = P(reg): Color_YELLOW;
	ds->color_num = P(num): Color_CYAN;
	ds->color_mov = P(mov): Color_WHITE;
	ds->color_invalid = P(invalid): Color_BRED;
	ds->color_gui_cflow = P(gui_cflow): Color_YELLOW;
	ds->color_gui_dataoffset = P(gui_dataoffset): Color_YELLOW;
	ds->color_gui_background = P(gui_background): Color_BLACK;
	ds->color_gui_alt_background = P(gui_alt_background): Color_GRAY;
	ds->color_gui_border = P(gui_border): Color_BGGRAY;
	ds->color_linehl = P(linehl): Color_BGBLUE;
	ds->color_func_var = P(func_var): Color_WHITE;
	ds->color_func_var_type = P(func_var_type): Color_BLUE;
	ds->color_func_var_addr = P(func_var_addr): Color_CYAN;

	ds->immstr = r_config_get_i (core->config, "asm.imm.str");
	ds->immtrim = r_config_get_i (core->config, "asm.imm.trim");
	ds->use_esil = r_config_get_i (core->config, "asm.esil");
	ds->pre_emu = r_config_get_i (core->config, "emu.pre");
	ds->show_flgoff = r_config_get_i (core->config, "asm.flags.offset");
	ds->show_nodup = r_config_get_i (core->config, "asm.nodup");
	{
		const char *ah = r_config_get (core->config, "asm.highlight");
		ds->asm_highlight = (ah && *ah)? r_num_math (core->num, ah): UT64_MAX;
	}
	ds->asm_anal = r_config_get_i (core->config, "asm.anal");
	ds->show_color = r_config_get_i (core->config, "scr.color");
	ds->show_color_bytes = r_config_get_i (core->config, "scr.color.bytes"); // maybe rename to asm.color.bytes
	ds->show_color_args = r_config_get_i (core->config, "scr.color.args");
	ds->colorop = r_config_get_i (core->config, "scr.color.ops"); // XXX confusing name // asm.color.inst (mnemonic + operands) ?
	ds->show_utf8 = r_config_get_i (core->config, "scr.utf8");
	ds->acase = r_config_get_i (core->config, "asm.ucase");
	ds->capitalize = r_config_get_i (core->config, "asm.capitalize");
	ds->atabs = r_config_get_i (core->config, "asm.tabs");
	ds->atabsonce = r_config_get_i (core->config, "asm.tabs.once");
	ds->atabsoff = r_config_get_i (core->config, "asm.tabs.off");
	ds->midflags = r_config_get_i (core->config, "asm.flags.middle");
	ds->midbb = r_config_get_i (core->config, "asm.bb.middle");
	ds->midcursor = r_config_get_i (core->config, "asm.midcursor");
	ds->decode = r_config_get_i (core->config, "asm.decode");
	core->parser->pseudo = ds->pseudo = r_config_get_i (core->config, "asm.pseudo");
	if (ds->pseudo) {
		ds->atabs = 0;
	}
	ds->filter = r_config_get_i (core->config, "asm.filter");
	ds->interactive = r_cons_is_interactive ();
	ds->jmpsub = r_config_get_i (core->config, "asm.jmpsub");
	ds->varsub = r_config_get_i (core->config, "asm.var.sub");
	core->parser->relsub = r_config_get_i (core->config, "asm.relsub");
	core->parser->localvar_only = r_config_get_i (core->config, "asm.var.subonly");
	core->parser->retleave_asm = NULL;
	ds->show_vars = r_config_get_i (core->config, "asm.var");
	ds->show_varsum = r_config_get_i (core->config, "asm.var.summary");
	ds->show_varaccess = r_config_get_i (core->config, "asm.var.access");
	ds->maxrefs = r_config_get_i (core->config, "asm.xrefs.max");
	ds->maxflags = r_config_get_i (core->config, "asm.maxflags");
	ds->asm_types = r_config_get_i (core->config, "asm.types");
	ds->foldxrefs = r_config_get_i (core->config, "asm.xrefs.fold");
	ds->show_lines = r_config_get_i (core->config, "asm.lines");
	ds->show_lines_bb = ds->show_lines ? r_config_get_i (core->config, "asm.lines.bb") : false;
	ds->linesright = r_config_get_i (core->config, "asm.lines.right");
	ds->show_indent = r_config_get_i (core->config, "asm.indent");
	ds->indent_space = r_config_get_i (core->config, "asm.indentspace");
	ds->tracespace = r_config_get_i (core->config, "asm.tracespace");
	ds->cyclespace = r_config_get_i (core->config, "asm.cyclespace");
	ds->show_dwarf = r_config_get_i (core->config, "asm.dwarf");
	ds->dwarfFile = r_config_get_i (ds->core->config, "asm.dwarf.file");
	ds->dwarfAbspath = r_config_get_i (ds->core->config, "asm.dwarf.abspath");
	ds->show_lines_call = ds->show_lines ? r_config_get_i (core->config, "asm.lines.call") : false;
	ds->show_lines_ret = ds->show_lines ? r_config_get_i (core->config, "asm.lines.ret") : false;
	ds->show_size = r_config_get_i (core->config, "asm.size");
	ds->show_trace = r_config_get_i (core->config, "asm.trace");
	ds->linesout = r_config_get_i (core->config, "asm.lines.out");
	ds->adistrick = r_config_get_i (core->config, "asm.middle"); // TODO: find better name
	ds->asm_demangle = r_config_get_i (core->config, "asm.demangle");
	ds->asm_describe = r_config_get_i (core->config, "asm.describe");
	ds->show_offset = r_config_get_i (core->config, "asm.offset");
	ds->show_offdec = r_config_get_i (core->config, "asm.decoff");
	ds->show_bbline = r_config_get_i (core->config, "asm.bbline");
	ds->show_section = r_config_get_i (core->config, "asm.section");
	ds->show_section_col = r_config_get_i (core->config, "asm.section.col");
	ds->show_section_perm = r_config_get_i (core->config, "asm.section.perm");
	ds->show_section_name = r_config_get_i (core->config, "asm.section.name");
	ds->show_symbols = r_config_get_i (core->config, "asm.symbol");
	ds->show_symbols_col = r_config_get_i (core->config, "asm.symbol.col");
	ds->asm_instr = r_config_get_i (core->config, "asm.instr");
	ds->show_emu = r_config_get_i (core->config, "asm.emu");
	ds->show_emu_str = r_config_get_i (core->config, "emu.str");
	ds->show_emu_stroff = r_config_get_i (core->config, "emu.str.off");
	ds->show_emu_strinv = r_config_get_i (core->config, "emu.str.inv");
	ds->show_emu_strflag = r_config_get_i (core->config, "emu.str.flag");
	ds->show_emu_strlea = r_config_get_i (core->config, "emu.str.lea");
	ds->show_emu_write = r_config_get_i (core->config, "emu.write");
	ds->show_emu_ssa = r_config_get_i (core->config, "emu.ssa");
	ds->show_emu_stack = r_config_get_i (core->config, "emu.stack");
	ds->stackFd = -1;
	if (ds->show_emu_stack) {
		// TODO: initialize fake stack in here
		const char *uri = "malloc://32K";
		ut64 size = r_num_get (core->num, "32K");
		ut64 addr = r_reg_getv (core->anal->reg, "SP") - (size / 2);
		emustack_min = addr;
		emustack_max = addr + size;
		ds->stackFd = r_io_fd_open (core->io, uri, R_PERM_RW, 0);
		RIOMap *map = r_io_map_add (core->io, ds->stackFd, R_PERM_RW, 0LL, addr, size);
		if (!map) {
			r_io_fd_close (core->io, ds->stackFd);
			eprintf ("Cannot create map for tha stack, fd %d got closed again\n", ds->stackFd);
			ds->stackFd = -1;
		} else {
			r_io_map_set_name (map, "fake.stack");
		}
	}
	ds->stackptr = core->anal->stackptr;
	ds->show_offseg = r_config_get_i (core->config, "asm.segoff");
	ds->show_flags = r_config_get_i (core->config, "asm.flags");
	ds->show_bytes = r_config_get_i (core->config, "asm.bytes");
	ds->asm_meta = r_config_get_i (core->config, "asm.meta");
	ds->show_reloff = r_config_get_i (core->config, "asm.reloff");
	ds->show_reloff_flags = r_config_get_i (core->config, "asm.reloff.flags");
	ds->show_lines_fcn = ds->show_lines ? r_config_get_i (core->config, "asm.lines.fcn") : false;
	ds->show_comments = r_config_get_i (core->config, "asm.comments");
	ds->show_usercomments = r_config_get_i (core->config, "asm.usercomments");
	ds->asm_hint_jmp = r_config_get_i (core->config, "asm.hint.jmp");
	ds->asm_hint_lea = r_config_get_i (core->config, "asm.hint.lea");
	ds->asm_hint_cdiv = r_config_get_i (core->config, "asm.hint.cdiv");
	ds->asm_hint_pos = r_config_get_i (core->config, "asm.hint.pos");
	ds->asm_hints = r_config_get_i (core->config, "asm.hints"); // only for cdiv wtf
	ds->show_slow = r_config_get_i (core->config, "asm.slow");
	ds->show_calls = r_config_get_i (core->config, "asm.calls");
	ds->show_family = r_config_get_i (core->config, "asm.family");
	ds->cmtcol = r_config_get_i (core->config, "asm.cmt.col");
	ds->show_cmtflgrefs = r_config_get_i (core->config, "asm.cmt.flgrefs");
	ds->show_cycles = r_config_get_i (core->config, "asm.cycles");
	ds->show_stackptr = r_config_get_i (core->config, "asm.stackptr");
	ds->show_xrefs = r_config_get_i (core->config, "asm.xrefs");
	ds->show_cmtrefs = r_config_get_i (core->config, "asm.cmt.refs");
	ds->cmtfold = r_config_get_i (core->config, "asm.cmt.fold");
	ds->show_cmtoff = r_config_get (core->config, "asm.cmt.off");
	ds->show_functions = r_config_get_i (core->config, "asm.functions");
	ds->nbytes = r_config_get_i (core->config, "asm.nbytes");
	ds->show_asciidot = !strcmp (core->print->strconv_mode, "asciidot");
	const char *strenc_str = r_config_get (core->config, "bin.str.enc");
	if (!strcmp (strenc_str, "latin1")) {
		ds->strenc = R_STRING_ENC_LATIN1;
	} else if (!strcmp (strenc_str, "utf8")) {
		ds->strenc = R_STRING_ENC_UTF8;
	} else if (!strcmp (strenc_str, "utf16le")) {
		ds->strenc = R_STRING_ENC_UTF16LE;
	} else if (!strcmp (strenc_str, "utf32le")) {
		ds->strenc = R_STRING_ENC_UTF32LE;
	} else {
		ds->strenc = R_STRING_ENC_GUESS;
	}
	core->print->bytespace = r_config_get_i (core->config, "asm.bytespace");
	ds->cursor = 0;
	ds->nb = 0;
	ds->flagspace_ports = r_flag_space_get (core->flags, "ports");
	ds->lbytes = r_config_get_i (core->config, "asm.lbytes");
	ds->show_comment_right_default = r_config_get_i (core->config, "asm.cmt.right");
	ds->show_comment_right = ds->show_comment_right_default;
	ds->show_flag_in_bytes = r_config_get_i (core->config, "asm.flags.inbytes");
	ds->show_marks = r_config_get_i (core->config, "asm.marks");
	ds->show_noisy_comments = r_config_get_i (core->config, "asm.noisy");
	ds->pre = DS_PRE_NONE;
	ds->ocomment = NULL;
	ds->linesopts = 0;
	ds->lastfail = 0;
	ds->ocols = 0;
	ds->lcols = 0;
	ds->printed_str_addr = UT64_MAX;
	ds->printed_flag_addr = UT64_MAX;

	ds->esil_old_pc = UT64_MAX;
	ds->esil_regstate = NULL;
	ds->esil_likely = false;

	ds->showpayloads = r_config_get_i (ds->core->config, "asm.payloads");
	ds->showrelocs = r_config_get_i (core->config, "bin.relocs");
	ds->min_ref_addr = r_config_get_i (core->config, "asm.var.submin");

	if (ds->show_flag_in_bytes) {
		ds->show_flags = 0;
	}
	if (r_config_get_i (core->config, "asm.lines.wide")) {
		ds->linesopts |= R_ANAL_REFLINE_TYPE_WIDE;
	}
	if (core->cons->vline) {
		if (ds->show_utf8) {
			ds->linesopts |= R_ANAL_REFLINE_TYPE_UTF8;
		}
	}
	if (ds->show_lines_bb) {
		ds->ocols += 10; // XXX
	}
	if (ds->show_offset) {
		ds->ocols += 14;
	}
	ds->lcols = ds->ocols + 2;
	if (ds->show_bytes) {
		ds->ocols += 20;
	}
	if (ds->show_trace) {
		ds->ocols += 8;
	}
	if (ds->show_stackptr) {
		ds->ocols += 4;
	}
	/* disasm */ ds->ocols += 20;
	ds->nb = ds->nbytes? (1 + ds->nbytes * 2): 0;
	ds->tries = 3;
	if (core->print->cur_enabled) {
		if (core->print->cur < 0) {
			core->print->cur = 0;
		}
		ds->cursor = core->print->cur;
	} else {
		ds->cursor = -1;
	}
	if (r_config_get_i (core->config, "asm.lines.wide")) {
		ds->linesopts |= R_ANAL_REFLINE_TYPE_WIDE;
	}
	if (core->cons->vline) {
		if (ds->show_utf8) {
			ds->linesopts |= R_ANAL_REFLINE_TYPE_UTF8;
		}
	}
	return ds;
}

static ut64 lastaddr = UT64_MAX;

static void ds_reflines_fini(RDisasmState *ds) {
	RAnal *anal = ds->core->anal;
	r_list_free (anal->reflines);
	r_list_free (anal->reflines2);
	anal->reflines = NULL;
	anal->reflines2 = NULL;
	R_FREE (ds->refline);
	R_FREE (ds->refline2);
}

static void ds_reflines_init(RDisasmState *ds) {
	RAnal *anal = ds->core->anal;

	lastaddr = UT64_MAX;

	if (ds->show_lines_bb) {
		ds_reflines_fini (ds);
		anal->reflines = r_anal_reflines_get (anal,
			ds->addr, ds->buf, ds->len, ds->l,
			ds->linesout, ds->show_lines_call);
		anal->reflines2 = r_anal_reflines_get (anal,
			ds->addr, ds->buf, ds->len, ds->l,
			ds->linesout, 1);
	} else {
		r_list_free (anal->reflines);
		r_list_free (anal->reflines2);
		anal->reflines = anal->reflines2 = NULL;
	}
}

#if 0
static void ds_reflines_fcn_init(RDisasmState *ds,  RAnalFunction *fcn, const ut8* buf) {
	RCore *core = ds->core;
	RAnal *anal = core->anal;
	if (ds->show_lines_bb) {
		// TODO: make anal->reflines implicit
		free (anal->reflines); // TODO: leak
		anal->reflines = r_anal_reflines_fcn_get (anal, fcn, -1, ds->linesout, ds->show_lines_call);
		free (anal->reflines2); // TODO: leak
		anal->reflines2 = r_anal_reflines_fcn_get (anal, fcn, -1, ds->linesout, 1);
	} else {
		r_list_free (anal->reflines);
		r_list_free (anal->reflines2);
		anal->reflines = anal->reflines2 = NULL;
	}
}
#endif

static void ds_free(RDisasmState *ds) {
	if (!ds) {
		return;
	}
	if (ds->show_emu_stack) {
		// TODO: destroy fake stack in here
		eprintf ("Free fake stack\n");
		if (ds->stackFd != -1) {
			r_io_fd_close (ds->core->io, ds->stackFd);
		}
	}
	r_asm_op_fini (&ds->asmop);
	r_anal_op_fini (&ds->analop);
	r_anal_hint_free (ds->hint);
	ds_print_esil_anal_fini (ds);
	ds_reflines_fini (ds);
	ds_print_esil_anal_fini (ds);
	sdb_free (ds->ssa);
	free (ds->comment);
	free (ds->line);
	free (ds->refline);
	free (ds->refline2);
	free (ds->opstr);
	free (ds->osl);
	free (ds->sl);
	free (ds->_tabsbuf);
	R_FREE (ds);
}

/* XXX move to r_print */
static char *colorize_asm_string(RCore *core, RDisasmState *ds, bool print_color) {
	char *spacer = NULL;
	char *source = ds->opstr? ds->opstr: r_asm_op_get_asm (&ds->asmop);
	char *hlstr = r_meta_get_string (ds->core->anal, R_META_TYPE_HIGHLIGHT, ds->at);
	bool partial_reset = line_highlighted (ds) ? true : ((hlstr && *hlstr) ? true : false);
	RAnalFunction *f = ds->show_color_args ? fcnIn (ds, ds->vat, R_ANAL_FCN_TYPE_NULL) : NULL;

	if (!ds->show_color || !ds->colorop) {
		return strdup (source);
	}

	if (print_color) {
		r_cons_strcat (r_print_color_op_type (core->print, ds->analop.type));
	}
	// workaround dummy colorizer in case of paired commands (tms320 & friends)
	spacer = strstr (source, "||");
	if (spacer) {
		char *scol1, *s1 = r_str_ndup (source, spacer - source);
		char *scol2, *s2 = strdup (spacer + 2);

		scol1 = r_print_colorize_opcode (ds->core->print, s1, ds->color_reg, ds->color_num, partial_reset, f ? f->addr : 0);
		free (s1);
		scol2 = r_print_colorize_opcode (ds->core->print, s2, ds->color_reg, ds->color_num, partial_reset, f ? f->addr : 0);
		free (s2);
		if (!scol1) {
			scol1 = strdup ("");
		}
		if (!scol2) {
			scol2 = strdup ("");
		}

		source = malloc (strlen (scol1) + strlen (scol2) + 2 + 1); // reuse source variable
		sprintf (source, "%s||%s", scol1, scol2);
		free (scol1);
		free (scol2);
		return source;
	}

	return r_print_colorize_opcode (ds->core->print, source, ds->color_reg, ds->color_num, partial_reset, f ? f->addr : 0);
}

static bool ds_must_strip(RDisasmState *ds) {
	if (ds && ds->strip && *ds->strip) {
		const char * optype = r_anal_optype_to_string (ds->analop.type);
		if (optype && *optype) {
			return strstr (ds->strip, optype);
		}
	}
	return false;
}

static void ds_highlight_word(RDisasmState * ds, char *word, char *color) {
	char *source = ds->opstr? ds->opstr: r_asm_op_get_asm (&ds->asmop);
	const char *color_reset = line_highlighted (ds) ? ds->color_linehl : Color_RESET_BG;
	char *asm_str = r_str_highlight (source, word, color, color_reset);
	ds->opstr = asm_str? asm_str: source;
}

static char *get_op_ireg (void *user, ut64 addr) {
	RCore *core = (RCore *)user;
	char *res = NULL;
	RAnalOp *op = r_core_anal_op (core, addr, 0);
	if (op && op->ireg) {
		res = strdup (op->ireg);
	}
	r_anal_op_free (op);
	return res;
}

static void ds_build_op_str(RDisasmState *ds, bool print_color) {
	RCore *core = ds->core;
	if (!ds->opstr) {
		ds->opstr = strdup (r_asm_op_get_asm (&ds->asmop));
	}
	/* initialize */
	core->parser->hint = ds->hint;
	core->parser->relsub = r_config_get_i (core->config, "asm.relsub");
	core->parser->relsub_addr = 0;
	if (ds->varsub && ds->opstr) {
		ut64 at = ds->vat;
		RAnalFunction *f = fcnIn (ds, at, R_ANAL_FCN_TYPE_NULL);
		core->parser->varlist = r_anal_var_list_dynamic;
		core->parser->get_op_ireg = get_op_ireg;
		r_parse_varsub (core->parser, f, at, ds->analop.size,
			ds->opstr, ds->strsub, sizeof (ds->strsub));
		if (*ds->strsub) {
			free (ds->opstr);
			ds->opstr = strdup (ds->strsub);
		}
		if (core->parser->relsub) {
			RList *list = r_anal_refs_get (core->anal, at);
			RListIter *iter;
			RAnalRef *ref;
			r_list_foreach (list, iter, ref) {
				if ((ref->type == R_ANAL_REF_TYPE_DATA
					|| ref->type == R_ANAL_REF_TYPE_STRING)
					&& ds->analop.type == R_ANAL_OP_TYPE_LEA) {
					core->parser->relsub_addr = ref->addr;
					break;
				}
			}
			r_list_free (list);
		}
	}
	char *asm_str = colorize_asm_string (core, ds, print_color);
	asm_str = ds_sub_jumps (ds, asm_str);
	if (ds->immtrim) {
		char *res = r_parse_immtrim (ds->opstr);
		if (res) {
			ds->opstr = res;
		}
		free (asm_str);
		return;
	}
	if (ds->decode) {
		char *tmpopstr = r_anal_op_to_string (core->anal, &ds->analop);
		// TODO: Use data from code analysis..not raw ds->analop here
		// if we want to get more information
		ds->opstr = tmpopstr? tmpopstr: asm_str? strdup (asm_str): strdup ("");
	} else {
		if (ds->hint && ds->hint->opcode) {
			free (ds->opstr);
			ds->opstr = strdup (ds->hint->opcode);
		}
		if (ds->filter) {
			RSpace *ofs = core->parser->flagspace;
			RSpace *fs = ds->flagspace_ports;
			if (ds->analop.type == R_ANAL_OP_TYPE_IO) {
				core->parser->notin_flagspace = NULL;
				core->parser->flagspace = fs;
			} else {
				if (fs) {
					core->parser->notin_flagspace = fs;
					core->parser->flagspace = fs;
				} else {
					core->parser->notin_flagspace = NULL;
					core->parser->flagspace = NULL;
				}
			}
			if (ds->analop.refptr) {
				if (core->parser->relsub_addr == 0) {
					ut64 killme = UT64_MAX;
					const int be = core->assembler->big_endian;
					r_io_read_i (core->io, ds->analop.ptr, &killme, ds->analop.refptr, be);
					core->parser->relsub_addr = killme;
				}
			}
			r_parse_filter (core->parser, ds->vat, core->flags, asm_str,
				ds->str, sizeof (ds->str), core->print->big_endian);
			core->parser->flagspace = ofs;
			free (ds->opstr);
			ds->opstr = strdup (ds->str);
		} else {
			free (ds->opstr);
			ds->opstr = strdup (asm_str? asm_str: "");
		}
	}
	if (ds->show_color) {
		int i = 0;
		char *word = NULL;
		char *bgcolor = NULL;
		char *wcdata = r_meta_get_string (ds->core->anal, R_META_TYPE_HIGHLIGHT, ds->at);
		int argc = 0;
		char **wc_array = r_str_argv (wcdata, &argc);
		for (i = 0; i < argc; i++) {
			bgcolor = strchr (wc_array[i], '\x1b');
			word = r_str_newlen (wc_array[i], bgcolor - wc_array[i]);
			ds_highlight_word (ds, word, bgcolor);
		}
	}
	if (ds->use_esil) {
		if (*R_STRBUF_SAFEGET (&ds->analop.esil)) {
			free (ds->opstr);
			ds->opstr = strdup (R_STRBUF_SAFEGET (&ds->analop.esil));
		} else {
			char *p = malloc (strlen (ds->opstr) + 6); /* What's up '\0' ? */
			if (p) {
				strcpy (p, "TODO,");
				strcpy (p + 5, ds->opstr);
				free (ds->opstr);
				ds->opstr = p;
			}
		}
	}
	free (asm_str);
}

//removed hints bits from since r_anal_build_range_on_hints along with
//r_core_seek_archbits will be used instead. The ranges are built from hints
R_API RAnalHint *r_core_hint_begin(RCore *core, RAnalHint* hint, ut64 at) {
	static char *hint_arch = NULL;
	static char *hint_syntax = NULL;
	r_anal_hint_free (hint);
	hint = r_anal_hint_get (core->anal, at);
	if (hint_arch) {
		r_config_set (core->config, "asm.arch", hint_arch);
		hint_arch = NULL;
	}
	if (hint_syntax) {
		r_config_set (core->config, "asm.syntax", hint_syntax);
		hint_syntax = NULL;
	}
	if (hint) {
		/* arch */
		if (hint->arch && !core->fixedarch) {
			if (!hint_arch) {
				hint_arch = strdup (r_config_get (core->config, "asm.arch"));
			}
			r_config_set (core->config, "asm.arch", hint->arch);
		}
		/* syntax */
		if (hint->syntax) {
			if (!hint_syntax) {
				hint_syntax = strdup (r_config_get (core->config, "asm.syntax"));
			}
			r_config_set (core->config, "asm.syntax", hint->syntax);
		}
		if (hint->high) {
			/* TODO: do something here */
		}
	}
	return hint;
}

static void ds_pre_line(RDisasmState *ds) {
	ds_setup_pre (ds, false, false);
	ds_print_pre (ds);
	char *tmp = ds->line;
	ds->line = ds->refline2;
	ds_print_lines_left (ds);
	ds->line = tmp;
}

static void ds_begin_line(RDisasmState *ds) {
	if (ds->pj) {
		pj_o (ds->pj);
		pj_kn (ds->pj, "offset", ds->vat);
		pj_k (ds->pj, "text");
	}
	ds->buf_line_begin = r_cons_get_buffer_len ();
}

static void ds_newline(RDisasmState *ds) {
	if (ds->pj) {
		pj_s (ds->pj, r_cons_get_buffer ());
		r_cons_reset ();
		pj_end (ds->pj);
	} else {
		r_cons_newline ();
	}
}

static void ds_show_refs(RDisasmState *ds) {
	RAnalRef *ref;
	RListIter *iter;
	RFlagItem *flagi, *flagat;

	if (!ds->show_cmtrefs) {
		return;
	}
	RList *list = r_anal_xrefs_get_from (ds->core->anal, ds->at);

	r_list_foreach (list, iter, ref) {
		char *cmt = r_meta_get_string (ds->core->anal, R_META_TYPE_COMMENT, ref->addr);
		flagi = r_flag_get_i (ds->core->flags, ref->addr);
		flagat = r_flag_get_at (ds->core->flags, ref->addr, false);
		// ds_align_comment (ds);
		if (ds->show_color) {
			r_cons_strcat (ds->color_comment);
		}
		if (flagi && flagat && (strcmp (flagi->name, flagat->name) != 0)) {
			_ds_comment_align_ (ds, true, false);
			ds_comment (ds, true, "; (%s)", flagi->name);
		}
		if (cmt) {
			_ds_comment_align_ (ds, true, false);
			ds_comment (ds, true, "; (%s)", cmt);
			free (cmt);
		}
		if (ref->type & R_ANAL_REF_TYPE_CALL) {
			RAnalOp aop;
			ut8 buf[12];
			r_io_read_at (ds->core->io, ref->at, buf, sizeof (buf));
			r_anal_op (ds->core->anal, &aop, ref->at, buf, sizeof (buf), R_ANAL_OP_MASK_ALL);
			if ((aop.type & R_ANAL_OP_TYPE_MASK) == R_ANAL_OP_TYPE_UCALL) {
				RAnalFunction * fcn = r_anal_get_fcn_at (ds->core->anal,
					ref->addr, R_ANAL_FCN_TYPE_NULL);
				_ds_comment_align_ (ds, true, false);
				if (fcn) {
					ds_comment (ds, true, "; %s", fcn->name);
				} else {
					ds_comment (ds, true, "; 0x%" PFMT64x"", ref->addr);
				}
			}
		}
		ds_print_color_reset (ds);
	}
	r_list_free (list);
}

static void ds_show_xrefs(RDisasmState *ds) {
	RAnalRef *refi;
	RListIter *iter, *it;
	RCore *core = ds->core;
	bool demangle = r_config_get_i (core->config, "bin.demangle");
	const char *lang = demangle ? r_config_get (core->config, "bin.lang") : NULL;
	char *name, *tmp;
	int count = 0;
	if (!ds->show_xrefs || !ds->show_comments) {
		return;
	}
	/* show xrefs */
	RList *xrefs = r_anal_xrefs_get (core->anal, ds->at);
	if (!xrefs) {
		return;
	}
	if (r_list_length (xrefs) > ds->maxrefs) {
		ds_begin_line (ds);
		ds_pre_xrefs (ds, false);
		ds_comment (ds, false, "%s; XREFS(%d)",
			ds->show_color? ds->pal_comment: "",
			r_list_length (xrefs));
		if (ds->show_color) {
			ds_print_color_reset (ds);
		}
		ds_newline (ds);
		r_list_free (xrefs);
		return;
	} else if (r_list_length (xrefs) > ds->foldxrefs) {
		int cols = r_cons_get_size (NULL);
		cols -= 15;
		cols /= 23;
		cols = cols > 5 ? 5 : cols;
		ds_begin_line (ds);
		ds_pre_xrefs (ds, false);
		ds_comment (ds, false, "%s; XREFS: ", ds->show_color? ds->pal_comment: "");
		r_list_foreach (xrefs, iter, refi) {
			ds_comment (ds, false, "%s 0x%08"PFMT64x"  ",
				r_anal_xrefs_type_tostring (refi->type), refi->addr);
			if (count == cols) {
				if (iter->n) {
					ds_print_color_reset (ds);
					ds_newline (ds);
					ds_begin_line (ds);
					ds_pre_xrefs (ds, false);
					ds_comment (ds, false, "%s; XREFS: ", ds->show_color? ds->pal_comment: "");
				}
				count = 0;
			} else {
				count++;
			}
		}
		ds_print_color_reset (ds);
		ds_newline (ds);
		r_list_free (xrefs);
		return;
	}

	RList *addrs = r_list_newf (free);
	RAnalFunction *fun, *next_fun;
	RFlagItem *f, *next_f;
	r_list_foreach (xrefs, iter, refi) {
		if (refi->at == ds->at) {
			fun = fcnIn (ds, refi->addr, -1);
			if (fun) {
				if (iter != xrefs->tail) {
					ut64 next_addr = ((RAnalRef *)(iter->n->data))->addr;
					next_fun = r_anal_get_fcn_in (core->anal, next_addr, -1);
					if (next_fun && next_fun->addr == fun->addr) {
						r_list_append (addrs, r_num_dup (refi->addr));
						continue;
					}
				}
				name = strdup (fun->name);
				r_list_append (addrs, r_num_dup (refi->addr));
			} else {
				f = r_flag_get_at (core->flags, refi->addr, true);
				if (f) {
					if (iter != xrefs->tail) {
						ut64 next_addr = ((RAnalRef *)(iter->n->data))->addr;
						next_f = r_flag_get_at (core->flags, next_addr, true);
						if (next_f && f->offset == next_f->offset) {
							r_list_append (addrs, r_num_dup (refi->addr - f->offset));
							continue;
						}
					}
					name = strdup (f->name);
					r_list_append (addrs, r_num_dup (refi->addr - f->offset));
				} else {
					name = strdup ("unk");
				}
			}
			if (demangle) {
				tmp = r_bin_demangle (core->bin->cur, lang, name, refi->addr);
				if (tmp) {
					free (name);
					name = tmp;
				}
			}
			ds_begin_line (ds);
			ds_pre_xrefs (ds, false);
			char* plural = r_list_length (addrs) > 1 ? "S" : "";
			char* plus = fun ? "" : "+";
			ds_comment (ds, false, "%s; %s XREF%s from %s (",
				COLOR (ds, pal_comment), r_anal_xrefs_type_tostring (refi->type), plural, name);
			ut64 *addrptr;
			r_list_foreach (addrs, it, addrptr) {
				if (addrptr && *addrptr) {
					ds_comment (ds, false, "%s%s0x%"PFMT64x, it == addrs->head ? "" : ", ", plus, *addrptr);
				}
			}
			ds_comment (ds, false, ")%s", COLOR_RESET (ds));
			ds_newline (ds);
			r_list_purge (addrs);
			R_FREE (name);
		} else {
			eprintf ("Corrupted database?\n");
		}
	}
	r_list_free (addrs);
	r_list_free (xrefs);
}

static void ds_atabs_option(RDisasmState *ds) {
	int n, i = 0, comma = 0, word = 0;
	int brackets = 0;
	char *t, *b;
	if (!ds || !ds->atabs) {
		return;
	}
	int bufasm_len = r_strbuf_length (&ds->asmop.buf_asm);
	int size = bufasm_len * (ds->atabs + 1) * 4;
	if (size < 1 || size < bufasm_len) {
		return;
	}
	free (ds->opstr);
	ds->opstr = b = malloc (size + 1);
	if (!b) {
		return;
	}
	strcpy (b, r_asm_op_get_asm (&ds->asmop));
	for (; *b; b++, i++) {
		if (*b == '(' || *b == '[') {
			brackets++;
		}
		if (*b == ')' || *b == ']') {
			brackets--;
		}
		if (*b == ',') {
			comma = 1;
		}
		if (*b != ' ') {
			continue;
		}
		if (word > 0 && !comma) {
			continue; //&& b[1]=='[') continue;
		}
		if (brackets > 0) {
			continue;
		}
		comma = 0;
		brackets = 0;
		n = (ds->atabs-i);
		t = strdup (b + 1); //XXX slow!
		if (n < 1) {
			n = 1;
		}
		memset (b, ' ', n);
		b += n;
		strcpy (b, t);
		free (t);
		i = 0;
		word++;
		if (ds->atabsonce) {
			break;
		}
	}
}

static int handleMidFlags(RCore *core, RDisasmState *ds, bool print) {
	int i;

	ds->hasMidflag = false;
	if (ds->midcursor && core->print->cur != -1) {
		ut64 cur = core->offset + core->print->cur;
		ut64 from = ds->at;
		ut64 to = ds->at + ds->oplen;
		if (cur > from && cur < to) {
			return cur - from;
		}
	}
	for (i = 1; i < ds->oplen; i++) {
		RFlagItem *fi = r_flag_get_i (core->flags, ds->at + i);
		if (fi && fi->name) {
			if (ds->midflags == 2 && ((fi->name[0] == '$') || (fi->realname && fi->realname[0] == '$'))) {
				i = 0;
			} else if (!strncmp (fi->name, "hit.", 4)) { // use search.prefix ?
				i = 0;
			} else if (!strncmp (fi->name, "str.", 4)) {
				ds->midflags = R_MIDFLAGS_REALIGN;
			} else if (!strncmp (fi->name, "reloc.", 6)) {
				if (print) {
					ds_begin_line (ds);
					// this reloc is displayed already as a flag comment
					// this is unnecessary imho
					r_cons_printf ("(%s)", fi->name);
					ds_newline (ds);
				}
				continue;
			} else if (ds->midflags == R_MIDFLAGS_SYMALIGN) {
				if (strncmp (fi->name, "sym.", 4)) {
					continue;
				}
			}
			ds->hasMidflag = true;
			return i;
		}
	}
	return 0;
}

static int handleMidBB(RCore *core, RDisasmState *ds) {
	int i;
	ds->hasMidbb = false;
	r_return_val_if_fail (core->anal, 0);
	// Unfortunately, can't just check the addr of the last insn byte since
	// a bb (and fcn) can be as small as 1 byte, and advancing i based on
	// bb->size is unsound if basic blocks can nest or overlap
	for (i = 1; i < ds->oplen; i++) {
		RAnalFunction *fcn = r_anal_get_fcn_in (core->anal, ds->at + i, 0);
		if (fcn) {
			RAnalBlock *bb = r_anal_fcn_bbget_in (core->anal, fcn, ds->at + i);
			if (bb && bb->addr > ds->at) {
				ds->hasMidbb = true;
				return bb->addr - ds->at;
			}
		}
	}
	return 0;
}

R_API int r_core_flag_in_middle(RCore *core, ut64 at, int oplen, int *midflags) {
	r_return_val_if_fail (midflags, 0);
	RDisasmState ds = {
		.at = at,
		.oplen = oplen,
		.midflags = *midflags
	};
	int ret = handleMidFlags (core, &ds, true);
	*midflags = ds.midflags;
	return ret;
}

R_API int r_core_bb_starts_in_middle(RCore *core, ut64 at, int oplen) {
	RDisasmState ds = {
		.at = at,
		.oplen = oplen
	};
	return handleMidBB (core, &ds);
}

static void ds_print_show_cursor(RDisasmState *ds) {
	RCore *core = ds->core;
	char res[] = "     ";
	void *p;
	if (!ds->show_marks) {
		return;
	}
	int q = core->print->cur_enabled &&
		ds->cursor >= ds->index &&
		ds->cursor < (ds->index + ds->asmop.size);
	p = r_bp_get_at (core->dbg->bp, ds->at);
	if (ds->midflags) {
		(void)handleMidFlags (core, ds, false);
	}
	if (ds->midbb) {
		(void)handleMidBB (core, ds);
	}
	if (p) {
		res[0] = 'b';
	}
	if (ds->hasMidflag || ds->hasMidbb) {
		res[1] = '~';
	}
	if (q) {
		if (ds->cursor == ds->index) {
			res[2] = '*';
		} else {
			int i = 2, diff = ds->cursor - ds->index;
			if (diff > 9) {
				res[i++] = '0' + (diff / 10);
			}
			res[i] = '0' + (diff % 10);
		}
	}
	r_cons_strcat (res);
}

static void ds_pre_xrefs(RDisasmState *ds, bool no_fcnlines) {
	ds_setup_pre (ds, false, false);
	if (ds->pre != DS_PRE_NONE && ds->pre != DS_PRE_EMPTY) {
		ds->pre = no_fcnlines ? DS_PRE_EMPTY : DS_PRE_FCN_MIDDLE;
	}
	ds_print_pre (ds);
	char *tmp = ds->line;
	ds->line = ds->refline2;
	ds_print_lines_left (ds);
	if (!ds->show_offset && ds->show_marks) {
		ds_print_show_cursor (ds);
	}
	ds->line = tmp;
}

static void ds_begin_comment(RDisasmState *ds) {
	if (ds->show_comment_right) {
		_ALIGN;
	} else {
		ds_begin_line (ds);
		ds_pre_xrefs (ds, false);
	}
}

//TODO: this function is a temporary fix. All analysis should be based on realsize. However, now for same architectures realisze is not used
static ut32 tmp_get_realsize (RAnalFunction *f) {
	ut32 size = r_anal_fcn_realsize (f);
	return (size > 0) ? size : r_anal_fcn_size (f);
}

static void ds_show_functions_argvar(RDisasmState *ds, RAnalVar *var, const char *base, bool is_var, char sign) {
	int delta = sign == '+' ? var->delta : -var->delta;
	const char *pfx = is_var ? "var" : "arg", *constr = NULL;
	RStrBuf *constr_buf = NULL;
	bool cond = false;
	if (ds->core && ds->core->anal) {
		constr_buf = var_get_constraint (ds->core->anal, var);
		if (constr_buf) {
			constr = r_strbuf_get (constr_buf);
			if (constr[0]) {
				cond = true;
			}
		}
	}
	r_cons_printf ("%s%s %s%s%s%s %s%s%s%s@ %s%c0x%x", COLOR_ARG (ds, color_func_var), pfx,
			COLOR_ARG (ds, color_func_var_type), var->type,
			r_str_endswith (var->type, "*") ? "" : " ",
			var->name, COLOR_ARG (ds, color_func_var_addr),
			cond? " { ":"",
			cond? constr: "",
			cond? "} ":"",
			base, sign, delta);
	r_strbuf_free (constr_buf);
}

static void printVarSummary(RDisasmState *ds, RList *list) {
	const char *numColor = ds->core->cons->context->pal.num;
	RAnalVar *var;
	RListIter *iter;
	int bp_vars = 0;
	int sp_vars = 0;
	int rg_vars = 0;
	int bp_args = 0;
	int sp_args = 0;
	int rg_args = 0;
	const char *bp_vars_color = COLOR_RESET (ds);
	const char *sp_vars_color = COLOR_RESET (ds);
	const char *rg_vars_color = COLOR_RESET (ds);
	const char *bp_args_color = COLOR_RESET (ds);
	const char *sp_args_color = COLOR_RESET (ds);
	const char *rg_args_color = COLOR_RESET (ds);
	r_list_foreach (list, iter, var) {
		if (var->isarg) {
			switch (var->kind) {
			case 'b':
				bp_args++;
				break;
			case 's':
				sp_args++;
				break;
			case 'r':
				rg_args++;
				break;
			}
		} else {
			switch (var->kind) {
			case 'b':
				bp_vars++;
				break;
			case 's':
				sp_vars++;
				break;
			case 'r':
				rg_vars++;
				break;
			}
		}
	}
	if (bp_vars) { bp_vars_color = numColor; }
	if (sp_vars) { sp_vars_color = numColor; }
	if (rg_vars) { rg_vars_color = numColor; }
	if (bp_args) { bp_args_color = numColor; }
	if (sp_args) { sp_args_color = numColor; }
	if (rg_args) { rg_args_color = numColor; }
	if (ds->show_varsum == 2) {
		ds_begin_line (ds);
		ds_print_pre (ds);
		r_cons_printf ("vars: %s%d%s %s%d%s %s%d%s",
				bp_vars_color, bp_vars, COLOR_RESET (ds),
				sp_vars_color, sp_vars, COLOR_RESET (ds),
				rg_vars_color, rg_vars, COLOR_RESET (ds));
		ds_newline (ds);
		ds_begin_line (ds);
		ds_print_pre (ds);
		r_cons_printf ("args: %s%d%s %s%d%s %s%d%s",
				bp_args_color, bp_args, COLOR_RESET (ds),
				sp_args_color, sp_args, COLOR_RESET (ds),
				rg_args_color, rg_args, COLOR_RESET (ds));
		ds_newline (ds);
		return;
	}
	ds_begin_line (ds);
	ds_print_pre (ds);
	r_cons_printf ("bp: %s%d%s (vars %s%d%s, args %s%d%s)",
			bp_args || bp_vars ? numColor : COLOR_RESET (ds), bp_args + bp_vars, COLOR_RESET (ds),
			bp_vars_color, bp_vars, COLOR_RESET (ds),
			bp_args_color, bp_args, COLOR_RESET (ds));
	ds_newline (ds);
	ds_begin_line (ds);
	ds_print_pre (ds);
	r_cons_printf ("sp: %s%d%s (vars %s%d%s, args %s%d%s)",
			sp_args || sp_vars ? numColor : COLOR_RESET (ds), sp_args+sp_vars, COLOR_RESET (ds),
			sp_vars_color, sp_vars, COLOR_RESET (ds),
			sp_args_color, sp_args, COLOR_RESET (ds));
	ds_newline (ds);
	ds_begin_line (ds);
	ds_print_pre (ds);
	r_cons_printf ("rg: %s%d%s (vars %s%d%s, args %s%d%s)",
			rg_args || rg_vars ? numColor : COLOR_RESET (ds), rg_args+rg_vars, COLOR_RESET (ds),
			rg_vars_color, rg_vars, COLOR_RESET (ds),
			rg_args_color, rg_args, COLOR_RESET (ds));
	ds_newline (ds);
}

static void ds_show_functions(RDisasmState *ds) {
	RAnalFunction *f;
	RCore *core = ds->core;
	char *fcn_name;
	bool fcn_name_alloc = false; // whether fcn_name needs to be freed by this function
	char *sign;

	if (!ds->show_functions) {
		return;
	}
	bool demangle = r_config_get_i (core->config, "bin.demangle");
	bool call = r_config_get_i (core->config, "asm.calls");
	const char *lang = demangle ? r_config_get (core->config, "bin.lang") : NULL;
	f = r_anal_get_fcn_in (core->anal, ds->at, R_ANAL_FCN_TYPE_NULL);
	if (!f || (f->addr != ds->at)) {
		return;
	}
	if (demangle) {
		fcn_name = r_bin_demangle (core->bin->cur, lang, f->name, f->addr);
		if (fcn_name) {
			fcn_name_alloc = true;
		} else {
			fcn_name = f->name;
		}
	} else {
		fcn_name = f->name;
	}

	ds_begin_line (ds);
	sign = r_anal_fcn_to_string (core->anal, f);
	if (f->type == R_ANAL_FCN_TYPE_LOC) {
		r_cons_printf ("%s%s ", COLOR (ds, color_fline),
			core->cons->vline[LINE_CROSS]); // |-
		r_cons_printf ("%s%s%s %d", COLOR (ds, color_floc),
			fcn_name, COLOR_RESET (ds), r_anal_fcn_size (f));
	} else {
		const char *fcntype;
		char cmt[32];
		get_bits_comment (core, f, cmt, sizeof (cmt));

		switch (f->type) {
		case R_ANAL_FCN_TYPE_FCN:
		case R_ANAL_FCN_TYPE_SYM:
			fcntype = "fcn"; break;
		case R_ANAL_FCN_TYPE_IMP:
			fcntype = "imp"; break;
		default:
			fcntype = "loc"; break;
		}
		//ds_set_pre (ds, core->cons->vline[CORNER_TL]);
		if (ds->show_lines_fcn) {
			ds->pre = DS_PRE_FCN_HEAD;
		}
		ds_print_pre (ds);
		if (ds->show_flgoff) {
			ds_print_lines_left (ds);
			ds_print_offset (ds);
		}
		r_cons_printf ("%s(%s) %s%s%s %d", COLOR (ds, color_fname),
			fcntype, fcn_name, cmt, COLOR_RESET (ds), tmp_get_realsize (f));
	}
	ds_newline (ds);
	if (sign) {
		ds_begin_line (ds);
		r_cons_printf ("// %s", sign);
		ds_newline (ds);
	}
	R_FREE (sign);
	if (ds->show_lines_fcn) {
		ds->pre = DS_PRE_FCN_MIDDLE;
	}
	ds->stackptr = core->anal->stackptr;
	RAnalFcnVarsCache vars_cache;
	r_anal_fcn_vars_cache_init (core->anal, &vars_cache, f);
	if (ds->show_vars && ds->show_varsum) {
		RList *all_vars = vars_cache.bvars;
		r_list_join (all_vars, vars_cache.svars);
		r_list_join (all_vars, vars_cache.rvars);
		printVarSummary (ds, all_vars);
	} else if (ds->show_vars) {
		char spaces[32];
		RAnalVar *var;
		RListIter *iter;
		if (call) {
			ds_begin_line (ds);
			r_cons_print (COLOR (ds, color_fline));
			ds_print_pre (ds);
			r_cons_printf ("%s  ", COLOR_RESET (ds));
			char *sig = r_anal_fcn_format_sig (core->anal, f, fcn_name, &vars_cache, COLOR (ds, color_fname), COLOR_RESET (ds));
			if (sig) {
				r_cons_print (sig);
				free (sig);
			}
			ds_newline (ds);
		}
		RList *all_vars = vars_cache.bvars;
		r_list_join (all_vars, vars_cache.svars);
		r_list_join (all_vars, vars_cache.rvars);
		r_list_foreach (all_vars, iter, var) {
			ds_begin_line (ds);
			int idx;
			RAnal *anal = ds->core->anal;
			memset (spaces, ' ', sizeof(spaces));
			idx = 12 - strlen (var->name);
			if (idx < 0) {
				idx = 0;
			}
			spaces[idx] = 0;
			ds_pre_xrefs (ds, false);

			if (ds->show_flgoff) {
				ds_print_offset (ds);
				r_cons_printf ("     ");
			}
			r_cons_printf ("%s; ", COLOR_ARG (ds, color_func_var));
			switch (var->kind) {
			case 'b': {
				char sign = var->delta > 0 ? '+' : '-';
				bool is_var = var->delta <= 0;
				ds_show_functions_argvar (ds, var,
					anal->reg->name[R_REG_NAME_BP], is_var, sign);
				}
				break;
			case 'r': {
				RRegItem *i = r_reg_index_get (anal->reg, var->delta);
				if (!i) {
					eprintf("Register not found");
					break;
				}
				r_cons_printf ("%sarg %s%s%s%s %s@ %s", COLOR_ARG (ds, color_func_var),
					COLOR_ARG (ds, color_func_var_type),
					var->type, r_str_endswith (var->type, "*") ? "" : " ",
					var->name, COLOR_ARG (ds, color_func_var_addr), i->name);
				}
				break;
			case 's': {
				bool is_var = !var->isarg;
				ds_show_functions_argvar (ds, var,
					anal->reg->name[R_REG_NAME_SP],
					is_var, '+');
				}
				break;
			}
			char *comment = r_meta_get_var_comment (anal, var->kind, var->delta, f->addr);
			if (comment) {
				r_cons_printf ("    %s; %s", COLOR (ds, color_comment), comment);
			}
			r_cons_print (COLOR_RESET (ds));
			ds_newline (ds);
		}
	}
	r_anal_fcn_vars_cache_fini (&vars_cache);
	if (fcn_name_alloc) {
		free (fcn_name);
	}
}

static void ds_setup_print_pre(RDisasmState *ds, bool tail, bool middle) {
	ds_setup_pre (ds, tail, middle);
	ds_print_pre (ds);
}

static void ds_setup_pre(RDisasmState *ds, bool tail, bool middle) {
	ds->cmtcount = 0;
	if (!ds->show_functions || !ds->show_lines_fcn) {
		ds->pre = DS_PRE_NONE;
		return;
	}
	ds->pre = DS_PRE_EMPTY;
	RAnalFunction *f = fcnIn (ds, ds->at, R_ANAL_FCN_TYPE_NULL);
	if (f) {
		if (f->addr == ds->at) {
			if (ds->analop.size == r_anal_fcn_size (f) && !middle) {
				ds->pre = DS_PRE_FCN_TAIL;
			} else {
				ds->pre = DS_PRE_FCN_MIDDLE;
			}
		} else if (f->addr + r_anal_fcn_size (f) - ds->analop.size == ds->at) {
			ds->pre = DS_PRE_FCN_TAIL;
		} else if (r_anal_fcn_is_in_offset (f, ds->at)) {
			ds->pre = DS_PRE_FCN_MIDDLE;
		}
		if (tail) {
			if (ds->pre == DS_PRE_FCN_TAIL) {
				ds->pre = DS_PRE_EMPTY;
			}
			if (ds->pre == DS_PRE_FCN_MIDDLE) {
				ds->pre = DS_PRE_FCN_TAIL;
			}
		}
	}
}

static void ds_print_pre(RDisasmState *ds) {
	RCore *core = ds->core;

	const char *c = NULL;
	switch(ds->pre) {
	case DS_PRE_FCN_HEAD:
		c = core->cons->vline[CORNER_TL];
		break;
	case DS_PRE_FCN_MIDDLE:
		c = core->cons->vline[LINE_VERT];
		break;
	case DS_PRE_FCN_TAIL:
		c = core->cons->vline[CORNER_BL];
		break;
	case DS_PRE_EMPTY:
		r_cons_print ("  ");
	case DS_PRE_NONE:
	default:
		return;
	}

	r_cons_printf ("%s%s%s ",
		COLOR (ds, color_fline), c,
		COLOR_RESET (ds));
}

//XXX review this with asm.cmt.right
static void ds_show_comments_right(RDisasmState *ds) {
	int linelen;
	RCore *core = ds->core;
	/* show comment at right? */
	int scr = ds->show_comment_right;
	if (!ds->show_comments && !ds->show_usercomments) {
		return;
	}
	RFlagItem *item = r_flag_get_i (core->flags, ds->at);
	char *comment = r_meta_get_string (core->anal, R_META_TYPE_COMMENT, ds->at);
	char *vartype = r_meta_get_string (core->anal, R_META_TYPE_VARTYPE, ds->at);
	if (!comment) {
		if (vartype) {
			ds->comment = r_str_newf ("%s; %s", COLOR_ARG (ds, color_func_var_type), vartype);
			free (vartype);
		} else if (item && item->comment && *item->comment) {
			ds->ocomment = item->comment;
			ds->comment = strdup (item->comment);
		}
	} else if (vartype) {
		ds->comment = r_str_newf ("%s; %s %s%s; %s", COLOR_ARG (ds, color_func_var_type), vartype, Color_RESET, COLOR (ds, color_usrcmt), comment);
		free (vartype);
		free (comment);
	} else {
		ds->comment = r_str_newf ("%s; %s", COLOR_ARG (ds, color_usrcmt), comment);
		free (comment);
	}
#if 0
	if (!ds->show_comments) {
		return;
	}
#endif
	if (!ds->comment || !*ds->comment) {
		return;
	}
	linelen = strlen (ds->comment) + 5;
	if (ds->show_comment_right_default) {
		if (ds->ocols + linelen < core->cons->columns) {
			if (!strchr (ds->comment, '\n')) { // more than one line?
				ds->show_comment_right = true;
			}
		}
	}
	if (!ds->show_comment_right) {
		ds_begin_line (ds);
		int mycols = ds->lcols;
		if ((mycols + linelen + 10) > core->cons->columns) {
			mycols = 0;
		}
		mycols /= 2;
		if (ds->show_color) {
			r_cons_strcat (ds->pal_comment);
		}
		/* print multiline comment */
		if (ds->cmtfold) {
			char *p = strdup (ds->comment);
			char *q = strchr (p, '\n');
			if (q) {
				*q = 0;
				r_cons_strcat (p);
				r_cons_strcat (" ; [z] unfold");
			}
			free (p);
		} else {
			ds_pre_xrefs (ds, false);
			if (ds->show_color) {
				r_cons_strcat (ds->color_usrcmt);
			}
			ds_comment (ds, false, "%s", ds->comment);
		}
		if (ds->show_color) {
			ds_print_color_reset (ds);
		}
		R_FREE (ds->comment);
		ds_newline (ds);
		/* flag one */
		if (item && item->comment && ds->ocomment != item->comment) {
			ds_begin_line (ds);
			if (ds->show_color) {
				r_cons_strcat (ds->pal_comment);
			}
			ds_newline (ds);
			ds_begin_line (ds);
			r_cons_strcat ("  ;  ");
			r_cons_strcat_justify (item->comment, mycols, ';');
			ds_newline (ds);
			if (ds->show_color) {
				ds_print_color_reset (ds);
			}
		}
	}
	ds->show_comment_right = scr;
}

static int flagCmp(const void *a, const void *b) {
	const RFlagItem *fa = a;
	const RFlagItem *fb = b;
	if (fa->realname && fb->realname) {
		return strcmp (fa->realname, fb->realname);
	}
	return strcmp (fa->name, fb->name);
}

static void ds_show_flags(RDisasmState *ds) {
	//const char *beginch;
	RFlagItem *flag;
	RListIter *iter;
	RAnalFunction *f = NULL;
	if (!ds->show_flags) {
		return;
	}
	RCore *core = ds->core;
	char addr[64];
	ut64 switch_addr = UT64_MAX;
	int case_start = -1, case_prev = 0, case_current = 0;
	f = fcnIn (ds, ds->at, R_ANAL_FCN_TYPE_NULL);
	const RList *flaglist = r_flag_get_list (core->flags, ds->at);
	RList *uniqlist = flaglist? r_list_uniq (flaglist, flagCmp): NULL;
	int count = 0;
	r_list_foreach (uniqlist, iter, flag) {
		if (f && f->addr == flag->offset && !strcmp (flag->name, f->name)) {
			// do not show flags that have the same name as the function
			continue;
		}
		bool no_fcn_lines = (f && f->addr == flag->offset);
		if (ds->maxflags && count >= ds->maxflags) {
			ds_pre_xrefs (ds, no_fcn_lines);
			r_cons_printf ("...\n");
			break;
		}
		count++;
		if (!strncmp (flag->name, "case.", 5)) {
			sscanf (flag->name + 5, "%63[^.].%d", addr, &case_current);
			ut64 saddr = r_num_math (core->num, addr);
			if (case_start == -1) {
				switch_addr = saddr;
				case_prev = case_current;
				case_start = case_current;
				if (iter != uniqlist->tail) {
					continue;
				}
			}
			if (case_current == case_prev + 1 && switch_addr == saddr) {
				case_prev = case_current;
				continue;
			}
		}
			ds_begin_line (ds);

		bool fake_flag_marks = (!ds->show_offset && ds->show_marks);
		if (ds->show_flgoff) {
			ds_pre_line (ds);
			ds_print_offset (ds);
			if (!fake_flag_marks) {
				r_cons_printf (" ");
			}
		} else {
			ds_pre_xrefs (ds, no_fcn_lines);
		}

		if (ds->show_color) {
			bool hasColor = false;
			if (flag->color) {
				char *color = r_cons_pal_parse (flag->color, NULL);
				if (color) {
					r_cons_strcat (color);
					free (color);
					ds->lastflag = flag;
					hasColor = true;
				}
			}
			if (!hasColor) {
				r_cons_strcat (ds->color_flag);
			}
		}

		if (!ds->show_flgoff || fake_flag_marks) {
			r_cons_printf (";-- ");
		}

		if (ds->asm_demangle && flag->realname) {
			if (!strncmp (flag->name, "case.", 5)) {
				if (!strncmp (flag->name + 5, "default", 7)) {
					r_cons_printf ("%s:", flag->name);
				} else if (case_prev != case_start) {
					r_cons_printf ("cases %d...%d (%s):", case_start, case_prev, addr);
					if (iter != uniqlist->head) {
						iter = iter->p;
					}
					case_start = case_current;
				} else {
					r_cons_printf ("case %d (%s):", case_prev, addr);
					case_start = -1;
				}
				case_prev = case_current;
			} else {
				const char *lang = r_config_get (core->config, "bin.lang");
				char *name = r_bin_demangle (core->bin->cur, lang, flag->realname, flag->offset);
				if (!name) {
					const char *n = flag->realname? flag->realname: flag->name;
					if (n) {
						name = strdup (n);
					}
				}
				if (name) {
					r_str_ansi_filter (name, NULL, NULL, -1);
					r_cons_printf ("%s:", name);
					R_FREE (name);
				}
			}
		} else {
			r_cons_printf ("%s:", flag->name);
		}
		if (ds->show_color) {
			r_cons_strcat (Color_RESET);
		}
		ds_newline (ds);
	}
	r_list_free (uniqlist);
}

static void ds_update_ref_lines(RDisasmState *ds) {
	if (ds->show_lines_bb) {
		free (ds->line);
		ds->line = r_anal_reflines_str (ds->core, ds->at, ds->linesopts);
		free (ds->refline);
		ds->refline = ds->line? strdup (ds->line): NULL;
		free (ds->refline2);
		ds->refline2 = r_anal_reflines_str (ds->core, ds->at,
			ds->linesopts | R_ANAL_REFLINE_TYPE_MIDDLE_BEFORE);
		if (ds->line) {
			if (strchr (ds->line, '<')) {
				ds->indent_level++;
			}
			if (strchr (ds->line, '>')) {
				ds->indent_level--;
			}
		} else {
			ds->indent_level = 0;
		}
	} else {
		R_FREE (ds->line);
		free (ds->refline);
		free (ds->refline2);
		ds->refline = strdup ("");
		ds->refline2 = strdup ("");
	}
}

static int ds_disassemble(RDisasmState *ds, ut8 *buf, int len) {
	RCore *core = ds->core;
	int ret;
	const char *info;
	Sdb *s = core->anal->sdb_meta;
	char key[100];
	ut64 mt_sz = UT64_MAX;

	//handle meta info to fix ds->oplen
	snprintf (key, sizeof (key) - 1, "meta.0x%"PFMT64x, ds->at);
	info = sdb_const_get (s, key, 0);
	if (info) {
		for (;*info; info++) {
			switch (*info) {
			case R_META_TYPE_DATA:
			case R_META_TYPE_STRING:
			case R_META_TYPE_FORMAT:
			case R_META_TYPE_MAGIC:
			case R_META_TYPE_HIDE:
				snprintf (key, sizeof (key) - 1,
						"meta.%c.0x%"PFMT64x, *info, ds->at);
				sdb_const_get (s, key, 0);
				mt_sz = sdb_array_get_num (s, key, 0, 0);
				//if (mt_sz) { break; }
				break;
			}
		}
	}

	if (ds->hint && ds->hint->size) {
		ds->oplen = ds->hint->size;
	}
	if (ds->hint && ds->hint->opcode) {
		free (ds->opstr);
		ds->opstr = strdup (ds->hint->opcode);
		return true;
	}
	r_asm_op_fini (&ds->asmop);
	ret = r_asm_disassemble (core->assembler, &ds->asmop, buf, len);
	if (ds->asmop.size < 1) {
		ds->asmop.size = 1;
	}
	// handle meta here //
	if (!ds->asm_meta) {
		int i = 0;
		// TODO: do in range
		RAnalMetaItem *meta = r_meta_find_in (core->anal, ds->at, R_META_TYPE_ANY, R_META_WHERE_HERE);
		if (meta && meta->size > 0) {
			// XXX this is just noise. should be rewritten
			switch (meta->type) {
			case R_META_TYPE_DATA:
				if (meta->str) {
					r_cons_printf (".data: %s\n", meta->str);
				}
				i += meta->size;
				break;
			case R_META_TYPE_STRING:
				i += meta->size;
				break;
			case R_META_TYPE_FORMAT:
				r_cons_printf (".format : %s\n", meta->str);
				i += meta->size;
				break;
			case R_META_TYPE_MAGIC:
				r_cons_printf (".magic : %s\n", meta->str);
				i += meta->size;
				break;
			case R_META_TYPE_RUN:
				r_core_cmd0 (core, meta->str);
				break;
			}
			int sz = R_MIN (16, meta->size - (ds->at - meta->from));
			ds->asmop.size = sz;
			r_asm_op_set_hexbuf (&ds->asmop, buf, sz);
			switch (meta->type) {
			case R_META_TYPE_STRING:
				r_asm_op_set_asm (&ds->asmop, sdb_fmt (".string \"%s\"", meta->str));
				break;
			// case R_META_TYPE_DATA:
			//	break;
			default:
				r_asm_op_set_asm (&ds->asmop, sdb_fmt (".hex %s", r_asm_op_get_hex (&ds->asmop)));
				break;
			}
			ds->oplen = sz; //ds->asmop.size;
			return i;
		}
	}

	if (ds->show_nodup) {
		const char *opname = (ret < 1)? "invalid": r_asm_op_get_asm (&ds->asmop);
		if (ds->prev_ins && !strcmp (ds->prev_ins, opname)) {
			if (!ds->prev_ins_eq) {
				ds->prev_ins_eq = true;
				r_cons_printf ("...");
			}
			ds->prev_ins_count++;
			return -31337;
		}
		if (ds->prev_ins_eq) {
			r_cons_printf ("dup (%d)\n", ds->prev_ins_count);
		}
		ds->prev_ins_count = 0;
		ds->prev_ins_eq = false;
		if (ds->prev_ins) {
			R_FREE (ds->prev_ins);
		}
		ds->prev_ins = strdup (r_asm_op_get_asm (&ds->asmop));
	}
	ds->oplen = ds->asmop.size;

	if (ret < 1) {
		ret = -1;
#if HASRETRY
		if (!ds->cbytes && ds->tries > 0) {
			ds->addr = core->assembler->pc;
			ds->tries--;
			ds->idx = 0;
			ds->retry = true;
			return ret;
		}
#endif
		ds->lastfail = 1;
		ds->asmop.size = (ds->hint && ds->hint->size) ? ds->hint->size : 1;
		ds->oplen = ds->asmop.size;
	} else {
		ds->lastfail = 0;
		ds->asmop.size = (ds->hint && ds->hint->size)
				? ds->hint->size
				: r_asm_op_get_size (&ds->asmop);
		ds->oplen = ds->asmop.size;
	}
	if (ds->pseudo) {
		r_parse_parse (core->parser, ds->opstr
				? ds->opstr
				: r_asm_op_get_asm (&ds->asmop),
				ds->str);
		free (ds->opstr);
		ds->opstr = strdup (ds->str);
	}
	if (ds->acase) {
		r_str_case (r_asm_op_get_asm (&ds->asmop), 1);
	} else if (ds->capitalize) {
		char *ba = r_asm_op_get_asm (&ds->asmop);
		*ba = toupper ((ut8)*ba);
	}
	if (info && mt_sz != UT64_MAX) {
		ds->oplen = mt_sz;
	}
	return ret;
}

static void ds_control_flow_comments(RDisasmState *ds) {
	if (ds->show_comments && ds->show_cmtflgrefs) {
		RFlagItem *item;
		if (ds->asm_anal) {
			switch (ds->analop.type) {
			case R_ANAL_OP_TYPE_CALL:
				r_core_cmdf (ds->core, "af @ 0x%"PFMT64x, ds->analop.jump);
				break;
			}
		}
		switch (ds->analop.type) {
		case R_ANAL_OP_TYPE_JMP:
		case R_ANAL_OP_TYPE_CJMP:
		case R_ANAL_OP_TYPE_CALL:
			item = r_flag_get_i (ds->core->flags, ds->analop.jump);
			if (item && item->comment) {
				if (ds->show_color) {
					r_cons_strcat (ds->pal_comment);
				}
				ds_align_comment (ds);
				r_cons_printf ("  ; ref to %s: %s\n", item->name, item->comment);
				ds_print_color_reset (ds);
			}
			break;
		}
	}
}

static void ds_print_lines_right(RDisasmState *ds){
	if (ds->linesright && ds->show_lines_bb && ds->line) {
		r_cons_printf ("%s%s%s", COLOR (ds, color_flow), ds->line, COLOR_RESET (ds));
	}
}

static void printCol(RDisasmState *ds, char *sect, int cols, const char *color) {
	int pre, post;
	if (cols < 8) {
		cols = 8;
	}
	int outsz = cols + 32;
	char *out = malloc (outsz);
	if (!out) {
		return;
	}
	memset (out, ' ', outsz);
	int sect_len = strlen (sect);

	if (sect_len > cols) {
		sect[cols-2] = '.';
		sect[cols-1] = '.';
		sect[cols] = 0;
	}
	if (ds->show_color) {
		pre = strlen (color) + 1;
		post = strlen (color) + 1 + strlen (Color_RESET);
		snprintf (out, outsz-pre, "%s %s", color, sect);
		strcat (out, Color_RESET);
		out[outsz-1] = 0;
	} else {
		strcpy (out + 1, sect);
		post = 0;
	}
	out[strlen (out)] = ' ';
	out[cols + post] = 0;
	r_cons_strcat (out);
	free (out);
}

static void ds_print_lines_left(RDisasmState *ds) {
	RCore *core = ds->core;

	if (ds->show_section) {
		char *str = NULL;
		if (ds->show_section_perm) {
			// iosections must die, this should be rbin_section_get
			RIOMap *map = r_io_map_get (core->io, ds->at);
			str = strdup (map? r_str_rwx_i (map->perm): "---");
		}
		if (ds->show_section_name) {
			str = r_str_appendf (str, " %s", get_section_name (core, ds->at));
		}
		char *sect = str? str: strdup ("");
		printCol (ds, sect, ds->show_section_col, ds->color_reg);
		free (sect);
	}
	if (ds->show_symbols) {
		static RFlagItem sfi = R_EMPTY;
		const char *name = "";
		int delta = 0;
		if (ds->fcn) {
			sfi.offset = ds->fcn->addr;
			sfi.name = ds->fcn->name;
			ds->lastflag = &sfi;
		} else {
			RFlagItem *fi = r_flag_get_at (core->flags, ds->at, false);
			if (fi) { // && (!ds->lastflag || fi->offset != ds->at))
				sfi.offset = fi->offset;
				sfi.name = fi->name;
				ds->lastflag = &sfi;
			}
		}
		if (ds->lastflag && ds->lastflag->name) {
			name = ds->lastflag->name;
			delta = ds->at - ds->lastflag->offset;
		}
		{
			char * str = r_str_newf ("%s + %-4d", name, delta);
			printCol (ds, str, ds->show_symbols_col, ds->color_num);
			free (str);
		}
	}
	if (ds->line) {
		if (ds->show_color) {
			if (!ds->linesright && ds->show_lines_bb) {
				r_cons_printf ("%s%s%s", COLOR (ds, color_flow), ds->line, COLOR_RESET (ds));
			}
		} else {
			r_cons_printf ("%s", ds->line);
		}
	}
}

static void ds_print_family(RDisasmState *ds) {
	if (ds->show_family) {
		const char *familystr = r_anal_op_family_to_string (ds->analop.family);
		r_cons_printf ("%5s ", familystr? familystr: "");
	}
}

static void ds_print_cycles(RDisasmState *ds) {
	if (ds->show_cycles) {
		if (!ds->analop.failcycles) {
			r_cons_printf ("%3d     ", ds->analop.cycles);
		} else {
			r_cons_printf ("%3d %3d ", ds->analop.cycles, ds->analop.failcycles);
		}
	}
	if (ds->cyclespace) {
		char spaces [32];
		int times = R_MIN (ds->analop.cycles/4, 30); // limit to 30
		memset (spaces, ' ', sizeof (spaces));
		spaces[times] = 0;
		r_cons_strcat (spaces);
	}
}

static void ds_update_stackptr(RDisasmState *ds, RAnalOp *op) {
	//ds->stackptr = ds->core->anal->stackptr;
	ds->ostackptr = ds->stackptr;
	switch (op->stackop) {
	case R_ANAL_STACK_RESET:
		ds->stackptr = 0;
		break;
	case R_ANAL_STACK_SET:
		ds->stackptr = op->stackptr;
		break;
	case R_ANAL_STACK_INC:
		ds->stackptr += op->stackptr;
		break;
	default:
		/* nothing to do here */
		break;
	}
	/* XXX if we reset the stackptr 'ret 0x4' has not effect.
	 * Use RAnalFunction->RAnalOp->stackptr? */
	if (op->type == R_ANAL_OP_TYPE_RET) {
		ds->stackptr = 0;
	}
//	ds->ostackptr = ds->stackptr;
	//ds->core->anal->stackptr = ds->stackptr;
}

static void ds_print_stackptr(RDisasmState *ds) {
	if (ds->show_stackptr) {
		r_cons_printf ("%5d%s", ds->stackptr,
			ds->analop.type == R_ANAL_OP_TYPE_CALL?">":
			ds->analop.stackop == R_ANAL_STACK_ALIGN? "=":
			ds->stackptr > ds->ostackptr? "+":
			ds->stackptr < ds->ostackptr? "-": " ");
		ds_update_stackptr (ds, &ds->analop);
	}
}

static void ds_print_offset(RDisasmState *ds) {
	RCore *core = ds->core;
	ut64 at = ds->vat;

	bool hasCustomColor = false;
	// probably tooslow
	RFlagItem *f = r_flag_get_at (core->flags, at, 1);
	if (ds->show_color && f) { // ds->lastflag) {
		const char *color = f->color;
		if (ds->at >= f->offset && ds->at < f->offset + f->size) {
		//	if (r_itv_inrange (f->itv, ds->at))
			if (color && *color) {
				char *k = r_cons_pal_parse (f->color, NULL);
				if (k) {
					r_cons_printf ("%s", k);
					hasCustomColor = true;
					free (k);
				}
			}
		}
	}
	r_print_set_screenbounds (core->print, at);
	if (ds->show_offset) {
		static RFlagItem sfi = R_EMPTY;
		const char *label = NULL;
		RFlagItem *fi;
		int delta = -1;
		bool show_trace = false;
		unsigned int seggrn = r_config_get_i (core->config, "asm.seggrn");

		if (ds->show_reloff) {
			RAnalFunction *f = r_anal_get_fcn_at (core->anal, at, R_ANAL_FCN_TYPE_NULL);
			if (!f) {
				f = fcnIn (ds, at, R_ANAL_FCN_TYPE_NULL); // r_anal_get_fcn_in (core->anal, at, R_ANAL_FCN_TYPE_NULL);
			}
			if (f) {
				delta = at - f->addr;
				sfi.name = f->name;
				sfi.offset = f->addr;
				ds->lastflag = &sfi;
				label = f->name;
			} else {
				if (ds->show_reloff_flags) {
					/* XXX: this is wrong if starting to disasm after a flag */
					fi = r_flag_get_i (core->flags, at);
					if (fi) {
						ds->lastflag = fi;
					}
					if (ds->lastflag) {
						if (ds->lastflag->offset == at) {
							delta = 0;
						} else {
							delta = at - ds->lastflag->offset;
						}
					} else {
						delta = at - core->offset;
					}
					if (ds->lastflag) {
						label = ds->lastflag->name;
					}
				}
			}
			if (!ds->lastflag) {
				delta = 0;
			}
		}
		if (ds->show_trace) {
			RDebugTracepoint *tp = r_debug_trace_get (ds->core->dbg, ds->at);
			show_trace = (tp? !!tp->count: false);
		}
		if (ds->hint && ds->hint->high) {
			show_trace = true;
		}
		if (hasCustomColor) {
			int of = core->print->flags;
			core->print->flags = 0;
			r_print_offset_sg (core->print, at, (at == ds->dest) || show_trace,
					ds->show_offseg, seggrn, ds->show_offdec, delta, label);
			core->print->flags = of;
			r_cons_strcat (Color_RESET);
		} else {
			r_print_offset_sg (core->print, at, (at == ds->dest) || show_trace,
					ds->show_offseg, seggrn, ds->show_offdec, delta, label);
		}
	}
	if (ds->atabsoff > 0 && ds->show_offset) {
		if (ds->_tabsoff != ds->atabsoff) {
			// TODO optimize to avoid down resizing
			char *b = malloc (ds->atabsoff + 1);
			if (b) {
				memset (b, ' ', ds->atabsoff);
				b[ds->atabsoff] = 0;
				free (ds->_tabsbuf);
				ds->_tabsbuf = b;
				ds->_tabsoff = ds->atabsoff;
			}
		}
		r_cons_strcat (ds->_tabsbuf);
	}
}

static void ds_print_op_size(RDisasmState *ds) {
	if (ds->show_size) {
		int size = ds->oplen;
		r_cons_printf ("%d ", size); //ds->analop.size);
	}
}

static void ds_print_trace(RDisasmState *ds) {
	RDebugTracepoint *tp = NULL;
	if (ds->show_trace) {
		tp = r_debug_trace_get (ds->core->dbg, ds->at);
		r_cons_printf ("%02x:%04x ", tp?tp->times:0, tp?tp->count:0);
	}
	if (ds->tracespace) {
		char spaces [32];
		int times;
		if (!tp) {
			tp = r_debug_trace_get (ds->core->dbg, ds->at);
		}
		if (tp) {
			times = R_MIN (tp->times, 30); // limit to 30
			memset (spaces, ' ', sizeof (spaces));
			spaces[times] = 0;
			r_cons_strcat (spaces);
		}
	}
}

static void ds_adistrick_comments(RDisasmState *ds) {
	if (ds->adistrick) {
		ds->middle = r_anal_reflines_middle (ds->core->anal,
			ds->core->anal->reflines, ds->at, ds->analop.size);
	}
}

static bool ds_print_data_type(RDisasmState *ds, const ut8 *buf, int ib, int size) {
	RCore *core = ds->core;
	const char *type = NULL;
	char msg[64];
	const int isSigned = (ib == 1 || ib == 8 || ib == 10)? 1: 0;
	switch (size) {
	case 1: type = isSigned? ".char": ".byte"; break;
	case 2: type = isSigned? ".int16": ".word"; break;
	case 3: type = "htons"; break;
	case 4: type = isSigned? ".int32": ".dword"; break;
	case 8: type = isSigned? ".int64": ".qword"; break;
	default: return false;
	}
	if (!ds->hinted_line) {
		r_cons_printf ("  ");
	}
	ut64 n = r_read_ble (buf, core->print->big_endian, size * 8);
	{
		int q = core->print->cur_enabled &&
			ds->cursor >= ds->index &&
			ds->cursor < (ds->index + size);
		if (q) {
			if (ds->cursor > ds->index) {
				int diff = ds->cursor - ds->index;
				r_cons_printf ("%d  ", diff);
			} else if (ds->cursor == ds->index) {
				r_cons_printf ("*  ");
			} else {
				r_cons_printf ("   ");
			}
		} else {
			r_cons_printf ("   ");
		}
	}

	r_cons_strcat (ds->color_mov);
	switch (ib) {
	case 1:
		r_str_bits (msg, buf, size * 8, NULL);
		r_cons_printf ("%s %sb", type, msg);
		break;
	case 3:
		r_cons_printf ("%s %d", type, ntohs (n & 0xFFFF));
		break;
	case 8:
		r_cons_printf ("%s %oo", type, n);
		break;
	case 10:
		r_cons_printf ("%s %d", type, n);
		break;
	default:
		switch (size) {
		case 1:
			r_cons_printf ("%s 0x%02x", type, n);
			break;
		case 2:
			r_cons_printf ("%s 0x%04x", type, n);
			break;
		case 4:
			r_cons_printf ("%s 0x%08x", type, n);
			break;
		case 8:
			r_cons_printf ("%s 0x%016" PFMT64x, type, n);
			break;
		default:
			return false;
		}
	}

	if (size == 4 || size == 8) {
		if (r_str_startswith (r_config_get (core->config, "asm.arch"), "arm")) {
			ut64 bits = r_config_get_i (core->config, "asm.bits");
			//adjust address for arm/thumb address
			if (bits < 64) {
				if (n & 1) {
					n--;
				}
			}
		}
		if (n >= ds->min_ref_addr) {
			const RList *flags = r_flag_get_list (core->flags, n);
			RListIter *iter;
			RFlagItem *fi;
			r_list_foreach (flags, iter, fi) {
				r_cons_printf (" ; %s", fi->name);
			}
		}
	}
	return true;
}

static int ds_print_meta_infos(RDisasmState *ds, ut8* buf, int len, int idx) {
	int ret = 0;
	const char *infos, *metas;
	char key[100];
	RAnalMetaItem MI, *mi = &MI;
	RCore *core = ds->core;
	Sdb *s = core->anal->sdb_meta;
	if (!ds->asm_meta) {
		return 0;
	}

	snprintf (key, sizeof (key), "meta.0x%" PFMT64x, ds->at);
	infos = sdb_const_get (s, key, 0);

	ds->mi_found = false;
	if (infos) {
		for (; *infos; infos++) {
			if (*infos == ',') {
				continue;
			}
			snprintf (key, sizeof (key), "meta.%c.0x%" PFMT64x, *infos, ds->at);
			metas = sdb_const_get (s, key, 0);
			if (!metas) {
				continue;
			}
			if (!r_meta_deserialize_val (core->anal, mi, *infos, ds->at, metas)) {
				continue;
			}
			// TODO: implement ranged meta find (if not at the begging of function..
			char *out = NULL;
			int hexlen;
			int delta;
			if (mi) {
				switch (mi->type) {
				case R_META_TYPE_STRING:
				{
					bool esc_bslash = core->print->esc_bslash;

					switch (mi->subtype) {
					case R_STRING_ENC_UTF8:
						out = r_str_escape_utf8 (mi->str, false, esc_bslash);
						break;
					case 0:  /* temporary legacy workaround */
						esc_bslash = false;
						/* fallthrough */
					default:
						out = r_str_escape_latin1 (mi->str, false, esc_bslash, false);
					}
					if (!out) {
						break;
					}
					r_cons_printf ("    .string %s\"%s\"%s ; len=%"PFMT64d,
							COLOR (ds, color_btext), out, COLOR_RESET (ds),
							mi->size);
					free (out);
					delta = ds->at - mi->from;
					ds->oplen = mi->size - delta;
					ds->asmop.size = (int)mi->size;
					//i += mi->size-1; // wtf?
					R_FREE (ds->line);
					R_FREE (ds->refline);
					R_FREE (ds->refline2);
					ds->mi_found = true;
					break;
				}
				case R_META_TYPE_HIDE:
					r_cons_printf ("(%"PFMT64d" bytes hidden)", mi->size);
					ds->asmop.size = mi->size;
					ds->oplen = mi->size;
					ds->mi_found = true;
					break;
				case R_META_TYPE_RUN:
					r_core_cmdf (core, "%s @ 0x%"PFMT64x, mi->str, ds->at);
					ds->asmop.size = mi->size;
					ds->oplen = mi->size;
					ds->mi_found = true;
					break;
				case R_META_TYPE_DATA:
					hexlen = len - idx;
					delta = ds->at - mi->from;
					if (mi->size < hexlen) {
						hexlen = mi->size;
					}
					ds->oplen = mi->size - delta;
					core->print->flags &= ~R_PRINT_FLAGS_HEADER;
					// TODO do not pass a copy in parameter buf that is possibly to small for this
					// print operation
					int size = R_MIN (mi->size, len - idx);
					if (!ds_print_data_type (ds, buf + idx, ds->hint? ds->hint->immbase: 0, size)) {
						r_cons_printf ("hex length=%" PFMT64d " delta=%d\n", size , delta);
						r_print_hexdump (core->print, ds->at, buf+idx, hexlen-delta, 16, 1, 1);
					}
					core->inc = 16; // ds->oplen; //
					core->print->flags |= R_PRINT_FLAGS_HEADER;
					ds->asmop.size = ret = (int)mi->size; //-delta;
					R_FREE (ds->line);
					R_FREE (ds->refline);
					R_FREE (ds->refline2);
					ds->mi_found = true;
					break;
				case R_META_TYPE_FORMAT:
					r_cons_printf ("format %s {\n", mi->str);
					r_print_format (core->print, ds->at, buf+idx, len-idx, mi->str, R_PRINT_MUSTSEE, NULL, NULL);
					r_cons_printf ("} %d", mi->size);
					ds->oplen = ds->asmop.size = ret = (int)mi->size;
					R_FREE (ds->line);
					R_FREE (ds->refline);
					R_FREE (ds->refline2);
					ds->mi_found = true;
					break;
				}
			}
			R_FREE (MI.str);
		}
	}
	return ret;
}

static void ds_instruction_mov_lea(RDisasmState *ds, int idx) {
	RCore *core = ds->core;
	RAnalValue *src;
	const int addrbytes = core->io->addrbytes;

	switch (ds->analop.type) {
	case R_ANAL_OP_TYPE_LENGTH:
	case R_ANAL_OP_TYPE_CAST:
	case R_ANAL_OP_TYPE_CMOV:
	case R_ANAL_OP_TYPE_MOV:
		src = ds->analop.src[0];
		if (src && src->memref > 0 && src->reg && core->anal->reg) {
			const char *pc = core->anal->reg->name[R_REG_NAME_PC];
			RAnalValue *dst = ds->analop.dst;
			if (dst && dst->reg && dst->reg->name) {
				if (src->reg->name && pc && !strcmp (src->reg->name, pc)) {
					RFlagItem *item;
					ut8 b[8];
					ut64 ptr = addrbytes * idx + ds->addr + src->delta + ds->analop.size;
					ut64 off = 0LL;
					r_io_read_at (core->io, ptr, b, src->memref);
					off = r_mem_get_num (b, src->memref);
					item = r_flag_get_i (core->flags, off);
					//TODO: introduce env for this print?
					r_cons_printf ("; MOV %s = [0x%"PFMT64x"] = 0x%"PFMT64x" %s\n",
							dst->reg->name, ptr, off, item?item->name: "");
					if (ds->asm_anal) {
						if (r_io_is_valid_offset (core->io, off, 0)) {
							r_anal_xrefs_set (core->anal, off, ds->addr, R_ANAL_REF_TYPE_DATA);
						}
					}
				}
			}
		}
		break;
// TODO: get from meta anal?
	case R_ANAL_OP_TYPE_LEA:
		src = ds->analop.src[0];
		if (src && src->reg && core->anal->reg && *(core->anal->reg->name)) {
			const char *pc = core->anal->reg->name[R_REG_NAME_PC];
			RAnalValue *dst = ds->analop.dst;
			if (dst && dst->reg && src->reg->name && pc && !strcmp (src->reg->name, pc)) {
				int index = 0;
				int memref = core->assembler->bits/8;
				RFlagItem *item;
				ut8 b[64];
				ut64 ptr = index + ds->addr + src->delta + ds->analop.size;
				ut64 off = 0LL;
				r_io_read_at (core->io, ptr, b, sizeof (b)); //memref);
				off = r_mem_get_num (b, memref);
				item = r_flag_get_i (core->flags, off);
				if (ds->asm_hint_lea) {
					char s[64];
					r_str_ncpy (s, (const char *)b, sizeof (s));
					r_str_filter (s, -1);
					ds_begin_comment (ds);
					ds_comment (ds, true, "; LEA %s = [0x%"PFMT64x"] = 0x%"PFMT64x" \"%s\"",
					            dst->reg->name, ptr, off, item?item->name: s);
					if (ds->asm_anal) {
						if (r_io_is_valid_offset (core->io, off, 0)) {
							r_anal_xrefs_set (core->anal, ptr, ds->addr, R_ANAL_REF_TYPE_DATA);
						}
					}
				}
			}
		}
	}
}

static st64 revert_cdiv_magic(st64 magic) {
	ut64 amagic = llabs (magic);
	const st64 N = ST64_MAX;
	st64 E, candidate;
	short s;

	if (amagic < 0xFFFFFF || amagic > UT32_MAX) {
		return 0;
	}
	if (magic < 0) {
		magic += 1LL << 32;
	}
	for (s = 0; s < 16; s++) {
		E = 1LL << (32 + s);
		candidate = (E + magic - 1) / magic;
		if (candidate > 0) {
			if ( ((N * magic) >> (32 + s)) == (N / candidate) ) {
				return candidate;
			}
		}
	}
	return 0;
}

static void ds_cdiv_optimization(RDisasmState *ds) {
	char *esil;
	char *end, *comma;
	st64 imm;
	st64 divisor;
	if (!ds->asm_hints || !ds->asm_hint_cdiv) {
		return;
	}
	switch (ds->analop.type) {
	case R_ANAL_OP_TYPE_MOV:
	case R_ANAL_OP_TYPE_MUL:
		esil = R_STRBUF_SAFEGET (&ds->analop.esil);
		while (esil) {
			comma = strstr (esil, ",");
			if (!comma) {
				break;
			}
			imm = strtol (esil, &end, 10);
			if (comma && comma == end) {
				divisor = revert_cdiv_magic (imm);
				if (divisor) {
					r_cons_printf (" ; CDIV: %lld * 2^n", divisor);
					break;
				}
			}
			esil = comma + 1;
		}
	}
	// /TODO: check following SHR instructions
}

static void ds_print_show_bytes(RDisasmState *ds) {
	RCore* core = ds->core;
	char *nstr, *str = NULL, pad[64];
	char *flagstr = NULL;
	int oldFlags = core->print->flags;
	char extra[128];
	int j, k;

	if (!ds->show_bytes || ds->nb < 1) {
		return;
	}
	if (!ds->show_color_bytes) {
		core->print->flags &= ~R_PRINT_FLAGS_COLOR;
	}
	strcpy (extra, " ");
	if (ds->show_flag_in_bytes) {
		flagstr = r_flag_get_liststr (core->flags, ds->at);
	}
	if (flagstr) {
		str = flagstr;
		if (ds->nb > 0) {
			k = ds->nb - strlen (flagstr) - 1;
			if (k < 0) {
				str[ds->nb - 1] = '\0';
			}
			if (k > sizeof (pad)) {
				k = 0;
			}
			for (j = 0; j < k; j++) {
				pad[j] = ' ';
			}
			pad[j] = '\0';
		} else {
			pad[0] = 0;
		}
	} else {
		if (ds->show_flag_in_bytes) {
			k = ds->nb - 1;
			if (k < 0 || k > sizeof (pad)) {
				k = 0;
			}
			for (j = 0; j < k; j++) {
				pad[j] = ' ';
			}
			pad[j] = '\0';
			str = strdup ("");
		} else {
			str = strdup (r_asm_op_get_hex (&ds->asmop));
			if (r_str_ansi_len (str) > ds->nb) {
				char *p = (char *)r_str_ansi_chrn (str, ds->nb);
				if (p)  {
					p[0] = '.';
					p[1] = '\0';
				}
			}
			ds->print->cur_enabled = (ds->cursor != -1);
			nstr = r_print_hexpair (ds->print, str, ds->index);
			if (ds->print->bytespace) {
				k = (ds->nb + (ds->nb / 2)) - r_str_ansi_len (nstr) + 2;
			} else {
				k = ds->nb - r_str_ansi_len (nstr) + 1;
			}
			if (k > 0) {
				// setting to sizeof screw up the disasm
				if (k > sizeof (pad)) {
					k = 18;
				}
				for (j = 0; j < k; j++) {
					pad[j] = ' ';
				}
				pad[j] = 0;
				if (ds->lbytes) {
					// hack to align bytes left
					strcpy (extra, pad);
					*pad = 0;
				}
			} else {
				pad[0] = 0;
			}
			free (str);
			str = nstr;
		}
	}
	r_cons_printf ("%s%s %s", pad, str, extra);
	free (str);
	core->print->flags = oldFlags;
}

static void ds_print_indent(RDisasmState *ds) {
	if (ds->show_indent) {
		char indent[128];
		int num = ds->indent_level * ds->indent_space;
		if (num < 0) {
			num = 0;
		}
		if (num >= sizeof (indent)) {
			num = sizeof (indent) - 1;
		}
		memset (indent, ' ', num);
		indent[num] = 0;
		r_cons_strcat (indent);
	}
}

static void ds_print_opstr(RDisasmState *ds) {
	if (ds->asm_instr) {
		ds_print_indent (ds);
		r_cons_strcat (ds->opstr);
		ds_print_color_reset (ds);
	}
}

static void ds_print_color_reset(RDisasmState *ds) {
	if (ds->show_color) {
		r_cons_strcat (Color_RESET);
	}
}

static int ds_print_middle(RDisasmState *ds, int ret) {
	if (ds->middle != 0) {
		ret -= ds->middle;
		ds_align_comment (ds);
		if (ds->show_color) {
			r_cons_strcat (ds->pal_comment);
		}
		r_cons_printf (" ; *middle* %d", ret);
		if (ds->show_color) {
			r_cons_strcat (Color_RESET);
		}
	}
	return ret;
}

static bool ds_print_labels(RDisasmState *ds, RAnalFunction *f) {
	RCore *core = ds->core;
	const char *label;
	if (!f) {
		// f = r_anal_get_fcn_in (core->anal, ds->at, 0);
		f = fcnIn (ds, ds->at, 0);
	}
	label = r_anal_fcn_label_at (core->anal, f, ds->at);
	if (!label) {
		return false;
	}
	ds_pre_line (ds);
	if (ds->show_color) {
		r_cons_strcat (ds->color_label);
		r_cons_printf (" .%s:\n", label);
		ds_print_color_reset (ds);
	} else {
		r_cons_printf (" .%s:\n", label);
	}
	return true;
}

#if 0
static void ds_print_import_name(RDisasmState *ds) {
	RListIter *iter = NULL;
	RBinReloc *rel = NULL;
	RCore * core = ds->core;

	switch (ds->analop.type) {
	case R_ANAL_OP_TYPE_JMP:
	case R_ANAL_OP_TYPE_CJMP:
	case R_ANAL_OP_TYPE_CALL:
		if (core->bin->cur->o->imports && core->bin->cur->o->relocs) {
			r_list_foreach (core->bin->cur->o->relocs, iter, rel) {
				if ((rel->vaddr == ds->analop.jump) &&
					(rel->import != NULL)) {
					if (ds->show_color) {
						r_cons_strcat (ds->color_fname);
					}
					// TODO: handle somehow ordinals import
					ds_align_comment (ds);
					r_cons_printf ("  ; (imp.%s)", rel->import->name);
					ds_print_color_reset (ds);
				}
			}
		}
	}
}
#endif

static void ds_print_sysregs(RDisasmState *ds) {
	RCore *core = ds->core;
	if (!ds->show_comments) {
		return;
	}
	switch (ds->analop.type) {
	// Syscalls first
	case R_ANAL_OP_TYPE_IO:
		{
			const int imm = (int)ds->analop.val;
			RSyscall *sc = core->anal->syscall;
			const char *ioname = r_syscall_get_io (sc, imm);
			if (ioname && *ioname) {
				_ALIGN;
				ds_comment (ds, true, "; IO %s", ioname);
				ds->has_description = true;
			}
		}
		break;
	// Then sysregs
	case R_ANAL_OP_TYPE_MOV:
	case R_ANAL_OP_TYPE_LEA:
	case R_ANAL_OP_TYPE_LOAD:
	case R_ANAL_OP_TYPE_STORE:
		{
			const int imm = (int)ds->analop.ptr;
			const char *sr = r_syscall_sysreg (core->anal->syscall, "reg", imm);
			if (sr) {
				_ALIGN;
				ds_comment (ds, true, "; REG %s - %s", sr, "");
				// TODO: add register description description
				ds->has_description = true;
			}
		}
		break;
	}
}

static void ds_print_fcn_name(RDisasmState *ds) {
	int delta;
	const char *label;
	RAnalFunction *f;
	RCore *core = ds->core;
	if (!ds->show_comments) {
		return;
	}
	switch (ds->analop.type) {
	case R_ANAL_OP_TYPE_JMP:
	case R_ANAL_OP_TYPE_CJMP:
	case R_ANAL_OP_TYPE_CALL:
		// f = r_anal_get_fcn_in (core->anal, ds->analop.jump, R_ANAL_FCN_TYPE_NULL);
		f = fcnIn (ds, ds->analop.jump, R_ANAL_FCN_TYPE_NULL);
		if (f && f->name && ds->opstr && !strstr (ds->opstr, f->name)) {
			// print label
			delta = ds->analop.jump - f->addr;
			label = r_anal_fcn_label_at (core->anal, f, ds->analop.jump);
			if (!ds->show_comment_right) {
				ds_begin_line (ds);
			}
			if (label) {
				_ALIGN;
				ds_comment (ds, true, "; %s.%s", f->name, label);
			} else {
				RAnalFunction *f2 = fcnIn (ds, ds->at, 0); //r_anal_get_fcn_in (core->anal, ds->at, 0);
				if (f != f2) {
					_ALIGN;
					if (delta > 0) {
						ds_comment (ds, true, "; %s+0x%x", f->name, delta);
					} else if (delta < 0) {
						ds_comment (ds, true, "; %s-0x%x", f->name, -delta);
					} else {
						ds_comment (ds, true, "; %s", f->name);
					}
				}
			}
		}
		break;
	}
}

static int ds_print_shortcut(RDisasmState *ds, ut64 addr, int pos) {
	char *shortcut = r_core_add_asmqjmp (ds->core, addr);
	int slen = shortcut? strlen (shortcut): 0;
	if (!pos && !shortcut) {
		//r_cons_printf (" ");
		//return 0;
	}
	if (pos) {
		ds_align_comment (ds);
	}
	const char *ch = pos? ";": "";
	if (ds->show_color) {
		r_cons_strcat (ds->pal_comment);
	}
	if (*ch) {
		slen++;
	}
	if (shortcut) {
		if (ds->core->is_asmqjmps_letter) {
			r_cons_printf ("%s[g%s]", ch, shortcut);
			slen++;
		} else {
			r_cons_printf ("%s[%s]", ch, shortcut);
		}
		free (shortcut);
	} else {
		r_cons_printf ("%s[?]", ch);
	}
	if (ds->show_color) {
		if (ds->core->print->resetbg) {
			r_cons_strcat (Color_RESET);
		} else {
			r_cons_strcat (Color_RESET_NOBG);
		}
	}
	slen++;
	return slen;
}

static bool ds_print_core_vmode_jump_hit(RDisasmState *ds, int pos) {
	RCore *core = ds->core;
	RAnal *a = core->anal;
	RAnalHint *hint = r_anal_hint_get (a, ds->at);
	if (hint) {
		if (hint->jump != UT64_MAX) {
			ds_print_shortcut (ds, hint->jump, pos);
		}
		r_anal_hint_free (hint);
		return true;
	}
	return false;
}

static void getPtr(RDisasmState *ds, ut64 addr, int pos) {
	ut8 buf[sizeof (ut64)] = {0};
	r_io_read_at (ds->core->io, addr, buf, sizeof (buf));
	if (ds->core->assembler->bits == 64) {
		ut64 n64 = r_read_ble64 (buf, 0);
		ds_print_shortcut (ds, n64, pos);
	} else {
		ut32 n32 = r_read_ble32 (buf, 0);
		ds_print_shortcut (ds, n32, pos);
	}
}

static void ds_print_core_vmode(RDisasmState *ds, int pos) {
	RCore *core = ds->core;
	bool gotShortcut = false;
	int i, slen = 0;

	if (!core->vmode) {
		return;
	}
	if (!ds->asm_hints) {
		return;
	}
	if (ds->asm_hint_lea) {
		RAnalMetaItem *mi = r_meta_find (ds->core->anal, ds->at, R_META_TYPE_ANY, R_META_WHERE_HERE);
		if (mi && mi->from) {
			int obits = ds->core->assembler->bits;
			ds->core->assembler->bits = mi->size * 8;
			getPtr (ds, mi->from, pos);
			ds->core->assembler->bits = obits;
			gotShortcut = true;
		}
	}
	switch (ds->analop.type) {
	case R_ANAL_OP_TYPE_UJMP:
	case R_ANAL_OP_TYPE_UJMP | R_ANAL_OP_TYPE_IND:
	case R_ANAL_OP_TYPE_UJMP | R_ANAL_OP_TYPE_IND | R_ANAL_OP_TYPE_COND:
	case R_ANAL_OP_TYPE_UJMP | R_ANAL_OP_TYPE_IND | R_ANAL_OP_TYPE_REG:
		if (ds->asm_hint_lea) {
			if (ds->analop.ptr != UT64_MAX && ds->analop.ptr != UT32_MAX) {
				getPtr (ds, ds->analop.ptr, pos);
				gotShortcut = true;
			}
		}
		break;
	case R_ANAL_OP_TYPE_MOV:
	case R_ANAL_OP_TYPE_LEA:
	case R_ANAL_OP_TYPE_LOAD:
		if (ds->asm_hint_lea) {
			if (ds->analop.ptr != UT64_MAX && ds->analop.ptr != UT32_MAX && ds->analop.ptr > 256) {
				slen = ds_print_shortcut (ds, ds->analop.ptr, pos);
				gotShortcut = true;
			}
		}
		break;
	case R_ANAL_OP_TYPE_UCALL:
	case R_ANAL_OP_TYPE_UCALL | R_ANAL_OP_TYPE_REG | R_ANAL_OP_TYPE_IND:
	case R_ANAL_OP_TYPE_UCALL | R_ANAL_OP_TYPE_IND:
#if 0
		if (ds->analop.jump == 0 && ds->analop.ptr) {
			ut8 buf[sizeof(ut64)] = {0};
			r_io_read_at (core->io, ds->analop.ptr, buf, sizeof (buf));
			ut32 n32 = r_read_ble32 (buf, 0);
			// is valid address
			// ut32 n64 = r_read_ble32 (buf, 0);
			ds_print_shortcut (ds, n32, pos);
		} else {
			// ds_print_shortcut (ds, ds->analop.jump, pos);
			ds_print_shortcut (ds, ds->analop.ptr, pos);
		}
#endif
		if (ds->asm_hint_jmp) {
			if (ds->analop.jump != UT64_MAX) {
				slen = ds_print_shortcut (ds, ds->analop.jump, pos);
			} else {
				slen = ds_print_shortcut (ds, ds->analop.ptr, pos);
			}
			gotShortcut = true;
		}
		break;
	case R_ANAL_OP_TYPE_RCALL:
		break;
	case R_ANAL_OP_TYPE_JMP:
	case R_ANAL_OP_TYPE_CJMP:
	case R_ANAL_OP_TYPE_CALL:
	case R_ANAL_OP_TYPE_COND | R_ANAL_OP_TYPE_CALL:
		if (ds->asm_hint_jmp) {
			slen = ds_print_shortcut (ds, ds->analop.jump, pos);
			gotShortcut = true;
		}
		break;
	default:
		if (ds_print_core_vmode_jump_hit (ds, pos)) {
			gotShortcut = true;
		}
		break;
	}
	int begin = (gotShortcut) ? (ds->asm_hint_pos == 0)? 1: 2: 3;
	for (i = begin - slen; i > 0; i--) {
		r_cons_strcat (" ");
	}
	ds->hinted_line = gotShortcut;
}

// align for comment
static void ds_align_comment(RDisasmState *ds) {
	if (!ds->show_comment_right_default) {
		return;
	}
	const int cmtcol = ds->cmtcol - 1;
	const char *ll = r_cons_get_buffer ();
	if (!ll) {
		return;
	}
	ll += ds->buf_line_begin;
	int cells = r_str_len_utf8_ansi (ll);
	int cols = ds->interactive ? ds->core->cons->columns : 1024;
	if (cells < cmtcol) {
		int len = cmtcol - cells;
		if (len < cols && len > 0) {
			r_cons_memset (' ', len);
		}
	}
	r_cons_print (" ");
}

static void ds_print_dwarf(RDisasmState *ds) {
	if (ds->show_dwarf) {
		int len = ds->opstr? strlen (ds->opstr): 0;
		if (len < 30) {
			len = 30 - len;
		}
		// TODO: cache value in ds
		int dwarfFile = (int)ds->dwarfFile + (int)ds->dwarfAbspath;
		free (ds->sl);
		ds->sl = r_bin_addr2text (ds->core->bin, ds->at, dwarfFile);
		if (ds->sl) {
			if ((!ds->osl || (ds->osl && strcmp (ds->sl, ds->osl)))) {
				char *chopstr, *line = strdup (ds->sl);
				if (!line) {
					return;
				}
				r_str_replace_char (line, '\t', ' ');
				r_str_replace_char (line, '\x1b', ' ');
				r_str_replace_char (line, '\r', ' ');
				r_str_replace_char (line, '\n', '\x00');
				chopstr = r_str_trim (line);
				if (!*chopstr) {
					free (line);
					return;
				}
				// handle_set_pre (ds, "  ");
				ds_align_comment (ds);
				if (ds->show_color) {
					r_cons_printf ("%s; %s"Color_RESET, ds->pal_comment, chopstr);
				} else {
					r_cons_printf ("; %s", chopstr);
				}
				free (ds->osl);
				ds->osl = ds->sl;
				ds->sl = NULL;
				free (line);
			}
		}
	}
}

static void ds_print_asmop_payload(RDisasmState *ds, const ut8 *buf) {
	if (ds->show_varaccess) {
		// XXX asume analop is filled
		//r_anal_op (core->anal, &ds->analop, ds->at, core->block+i, core->blocksize-i);
		int v = ds->analop.ptr;
		switch (ds->analop.stackop) {
		case R_ANAL_STACK_GET:
			if (v < 0) {
				r_cons_printf (" ; local.get %d", -v);
			} else {
				r_cons_printf (" ; arg.get %d", v);
			}
			break;
		case R_ANAL_STACK_SET:
			if (v < 0) {
				r_cons_printf (" ; local.set %d", -v);
			} else {
				r_cons_printf (" ; arg.set %d", v);
			}
			break;
		}
	}
	if (ds->asmop.payload != 0) {
		r_cons_printf ("\n; .. payload of %d byte(s)", ds->asmop.payload);
		if (ds->showpayloads) {
			int mod = ds->asmop.payload % ds->core->assembler->dataalign;
			int x;
			for (x = 0; x < ds->asmop.payload; ++x) {
				r_cons_printf ("\n        0x%02x", buf[ds->oplen + x]);
			}
			for (x = 0; x < mod; ++x) {
				r_cons_printf ("\n        0x%02x ; alignment", buf[ds->oplen + ds->asmop.payload + x]);
			}
		}
	}
}

/* Do not use this function for escaping JSON! */
static char *ds_esc_str(RDisasmState *ds, const char *str, int len, const char **prefix_out, bool is_comment) {
	int str_len;
	char *escstr = NULL;
	const char *prefix = "";
	bool esc_bslash = ds->core->print->esc_bslash;
	switch (ds->strenc) {
	case R_STRING_ENC_LATIN1:
		escstr = r_str_escape_latin1 (str, ds->show_asciidot, esc_bslash, is_comment);
		break;
	case R_STRING_ENC_UTF8:
		escstr = r_str_escape_utf8 (str, ds->show_asciidot, esc_bslash);
		break;
	case R_STRING_ENC_UTF16LE:
		escstr = r_str_escape_utf16le (str, len, ds->show_asciidot, esc_bslash);
		prefix = "u";
		break;
	case R_STRING_ENC_UTF32LE:
		escstr = r_str_escape_utf32le (str, len, ds->show_asciidot, esc_bslash);
		prefix = "U";
		break;
	default:
		str_len = strlen (str);
		if ((str_len == 1 && len > 3 && str[2] && !str[3])
		    || (str_len == 3 && len > 5 && !memcmp (str, "\xff\xfe", 2) && str[4] && !str[5])) {
			escstr = r_str_escape_utf16le (str, len, ds->show_asciidot, esc_bslash);
			prefix = "u";
		} else if (str_len == 1 && len > 7 && !str[2] && !str[3] && str[4] && !str[5]) {
			RStrEnc enc = R_STRING_ENC_UTF32LE;
			RRune ch;
			const char *ptr, *end;
			end = (const char *)r_mem_mem_aligned ((ut8 *)str, len, (ut8 *)"\0\0\0\0", 4, 4);
			if (!end) {
				end = str + len - 1;
			}
			for (ptr = str; ptr < end; ptr += 4) {
				if (r_utf32le_decode ((ut8 *)ptr, end - ptr, &ch) > 0 && ch > 0x10ffff) {
					enc = R_STRING_ENC_LATIN1;
					break;
				}
			}
			if (enc == R_STRING_ENC_UTF32LE) {
				escstr = r_str_escape_utf32le (str, len, ds->show_asciidot, esc_bslash);
				prefix = "U";
			} else {
				escstr = r_str_escape_latin1 (str, ds->show_asciidot, esc_bslash, is_comment);
			}
		} else {
			RStrEnc enc = R_STRING_ENC_LATIN1;
			const char *ptr = str, *end = str + str_len;
			for (; ptr < end; ptr++) {
				if (r_utf8_decode ((ut8 *)ptr, end - ptr, NULL) > 1) {
					enc = R_STRING_ENC_UTF8;
					break;
				}
			}
			escstr = (enc == R_STRING_ENC_UTF8 ?
				r_str_escape_utf8 (str, ds->show_asciidot, esc_bslash) :
				r_str_escape_latin1 (str, ds->show_asciidot, esc_bslash, is_comment));
		}
	}
	if (prefix_out) {
		*prefix_out = prefix;
	}
	return escstr;
}

static void ds_print_str(RDisasmState *ds, const char *str, int len, ut64 refaddr) {
	if (ds->core->flags->realnames || !r_bin_string_filter (ds->core->bin, str, refaddr)) {
		return;
	}
	const char *prefix;
	char *escstr = ds_esc_str (ds, str, len, &prefix, false);
	if (escstr) {
		bool inv = ds->show_color && !ds->show_emu_strinv;
		ds_begin_comment (ds);
		ds_comment (ds, true, "; %s%s\"%s\"%s", inv ? Color_INVERT : "", prefix, escstr,
		            inv ? Color_INVERT_RESET : "");
		ds->printed_str_addr = refaddr;
		free (escstr);
	}
}

static inline bool is_filtered_flag(RDisasmState *ds, const char *name) {
	if (ds->show_noisy_comments || strncmp (name, "str.", 4)) {
		return false;
	}
	ut64 refaddr = ds->analop.ptr;
	char *anal_flag = r_meta_get_string (ds->core->anal, R_META_TYPE_STRING, refaddr);
	if (anal_flag) {
		anal_flag = strdup (anal_flag);
		if (anal_flag) {
			r_name_filter (anal_flag, -1);
			if (!strcmp (&name[4], anal_flag)) {
				free (anal_flag);
				return true;
			}
			free (anal_flag);
		}
	}
	return false;
}

/* convert numeric value in opcode to ascii char or number */
static void ds_print_ptr(RDisasmState *ds, int len, int idx) {
	RCore *core = ds->core;
	ut64 p = ds->analop.ptr;
	ut64 v = ds->analop.val;
	ut64 refaddr = p;
	bool aligned = false;
	int refptr = ds->analop.refptr;
	RFlagItem *f = NULL, *f2 = NULL;
	bool f2_in_opstr = false;  /* Also if true, f exists */
	if (!ds->show_comments || !ds->show_slow) {
		return;
	}

	ds->chref = 0;
	if ((char)v > 0 && v >= '!') {
		ds->chref = (char)v;
		if (ds->immstr) {
			char *str = r_str_from_ut64 (r_read_ble64 (&v, core->print->big_endian));
			if (str && *str) {
				const char *ptr = str;
				bool printable = true;
				for (; *ptr; ptr++) {
					if (!IS_PRINTABLE (*ptr)) {
						printable = false;
						break;
					}
				}
				if (r_flag_get_i (core->flags, v)) {
					printable = false;
				}
				if (printable) {
					ds_begin_comment (ds);
					ds_comment (ds, true, "; '%s'", str);
				}
			}
			free (str);
		} else {
			if ((char)v > 0 && v >= '!' && v <= '~') {
				ds_begin_comment (ds);
				aligned = true;
				ds_comment (ds, true, "; '%c'", (char)v);
			}
		}
	}
	RList *list = NULL;
	RListIter *iter;
	RAnalRef *ref;
	list = r_anal_refs_get (core->anal, ds->at);
	r_list_foreach (list, iter, ref) {
		if (ref->type == R_ANAL_REF_TYPE_STRING || ref->type == R_ANAL_REF_TYPE_DATA) {
			if ((f = r_flag_get_i (core->flags, ref->addr))) {
				refaddr = ref->addr;
				break;
			}
		}
	}
	r_list_free (list);
	if (ds->analop.type == (R_ANAL_OP_TYPE_MOV | R_ANAL_OP_TYPE_REG)
	    && ds->analop.stackop == R_ANAL_STACK_SET
	    && ds->analop.val != UT64_MAX && ds->analop.val > 10) {
		const char *arch = r_config_get (core->config, "asm.arch");
		if (arch && !strcmp (arch, "x86")) {
			p = refaddr = ds->analop.val;
			refptr = 0;
		}
	}
	bool flag_printed = false;
	bool refaddr_printed = false;
	bool string_printed = false;
	if (refaddr == UT64_MAX) {
		/* do nothing */
	} else if (((st64)p) > 0 || ((st64)refaddr) > 0) {
		const char *kind;
		char *msg = calloc (sizeof (char), len);
		if (((st64)p) > 0) {
			f = r_flag_get_i (core->flags, p);
			if (f) {
				ut64 relsub_addr = core->parser->relsub_addr;
				if (relsub_addr && relsub_addr != p) {
					f2 = r_flag_get_i2 (core->flags, relsub_addr);
					if (!f2) {
						f2 = r_flag_get_i (core->flags, relsub_addr);
					}
					f2_in_opstr = f2 && ds->opstr && strstr (ds->opstr, f2->name);
				}
				refaddr = p;
				if (!flag_printed && !is_filtered_flag (ds, f->name)
				    && (!ds->opstr || !strstr (ds->opstr, f->name))
				    && !f2_in_opstr) {
					ds_begin_comment (ds);
					ds_comment (ds, true, "; %s", f->name);
					ds->printed_flag_addr = p;
					flag_printed = true;
				}
			}
		}
		r_io_read_at (core->io, refaddr, (ut8*)msg, len - 1);
		if (refptr) {
			ut64 num = r_read_ble (msg, core->print->big_endian, refptr * 8);
			st64 n = (st64)num;
			st32 n32 = (st32)(n & UT32_MAX);
			if (ds->analop.type == R_ANAL_OP_TYPE_LEA) {
				char str[128] = {0};
				f = r_flag_get_i (core->flags, refaddr);
				if (!f && ds->show_slow) {
					r_io_read_at (ds->core->io, ds->analop.ptr,
						      (ut8 *)str, sizeof (str) - 1);
					str[sizeof (str) - 1] = 0;
					if (!string_printed && str[0] && r_str_is_printable_incl_newlines (str)) {
						ds_print_str (ds, str, sizeof (str), ds->analop.ptr);
						string_printed = true;
					}
				}
			} else {
				if (n == UT32_MAX || n == UT64_MAX) {
					ds_begin_comment (ds);
					ds_comment (ds, true, "; [0x%" PFMT64x":%d]=-1",
							refaddr, refptr);
				} else if (n == n32 && (n32 > -512 && n32 < 512)) {
					ds_begin_comment (ds);
					ds_comment (ds, true, "; [0x%" PFMT64x
							  ":%d]=%"PFMT64d, refaddr, refptr, n);
				} else {
					const char *kind, *flag = "";
					char *msg2 = NULL;
					RFlagItem *f2_ = r_flag_get_i (core->flags, n);
					if (f2_) {
						flag = f2_->name;
					} else {
						msg2 = calloc (sizeof (char), len);
						r_io_read_at (core->io, n, (ut8*)msg2, len - 1);
						msg2[len - 1] = 0;
						kind = r_anal_data_kind (core->anal, refaddr, (const ut8*)msg2, len - 1);
						if (kind && !strcmp (kind, "text")) {
							r_str_filter (msg2, 0);
							if (*msg2) {
								char *lala = r_str_newf ("\"%s\"", msg2);
								free (msg2);
								flag = msg2 = lala;
							}
						}
					}
					ds_begin_comment (ds);
					{
						const char *refptrstr = "";
						if (core->print->flags & R_PRINT_FLAGS_SECSUB) {
							RBinObject *bo = r_bin_cur_object (core->bin);
							RBinSection *s = bo? r_bin_get_section_at (bo, n, core->io->va): NULL;
							if (s) {
								refptrstr = s->name;
							}
						}
						ds_comment_start (ds, "; [");
						if (f && f2_in_opstr) {
							ds_comment_middle (ds, "%s", f->name);
							flag_printed = true;
						} else {
							ds_comment_middle (ds, "0x%" PFMT64x, refaddr);
						}
						ds_comment_end (ds, ":%d]=%s%s0x%" PFMT64x "%s%s",
								refptr, refptrstr, *refptrstr ? "." : "",
								n, (flag && *flag) ? " " : "", flag);
					}
					free (msg2);
				}
				refaddr_printed = true;
			}
		}
		if (!strcmp (ds->show_cmtoff, "true")) {
			ds_begin_comment (ds);
			ds_comment (ds, true, "; 0x%" PFMT64x, refaddr);
			refaddr_printed = true;
		} else if (!refaddr_printed && strcmp (ds->show_cmtoff, "false")) {
			char addrstr[32] = {0};
			snprintf (addrstr, sizeof (addrstr), "0x%" PFMT64x, refaddr);
			if (!ds->opstr || !strstr (ds->opstr, addrstr)) {
				snprintf (addrstr, sizeof (addrstr), "0x%08" PFMT64x, refaddr);
				if (!ds->opstr || !strstr (ds->opstr, addrstr)) {
					bool print_refaddr = true;
					if (refaddr < 10) {
						snprintf (addrstr, sizeof (addrstr), "%" PFMT64u, refaddr);
						if (ds->opstr && strstr (ds->opstr, addrstr)) {
							print_refaddr = false;
						}
					}
					if (print_refaddr) {
						if (!aligned) {
							ds_begin_comment (ds);
						}
						ds_comment (ds, true, "; 0x%" PFMT64x, refaddr);
						refaddr_printed = true;
					}
				}
			}
		}
#if 1
		if (!(IS_PRINTABLE (*msg) || IS_WHITECHAR (*msg)
		      || (len > 1 && !memcmp (msg, "\xff\xfe", 2)))) {
			*msg = 0;
		} else {
			msg[len - 1] = 0;
		}
#endif
		f = r_flag_get_i (core->flags, refaddr);
		if (f) {
			if (strlen (msg) != 1) {
				char *msg2 = r_str_new (msg);
				if (msg2) {
					r_str_filter (msg2, 0);
					if (!strncmp (msg2, "UH..", 4)) {
						*msg = 0;
					}
					free (msg2);
				}
			}
			if (*msg) {
				if (!string_printed) {
					ds_print_str (ds, msg, len, refaddr);
					string_printed = true;
				}
			} else if (!flag_printed && (!ds->opstr || !strstr (ds->opstr, f->name))) {
				ds_begin_comment (ds);
				ds_comment (ds, true, "; %s", f->name);
				ds->printed_flag_addr = refaddr;
				flag_printed = true;
			}
		} else {
			if (refaddr == UT64_MAX || refaddr == UT32_MAX) {
				ds_begin_comment (ds);
				ds_comment (ds, true, "; -1");
			} else if (((char)refaddr > 0) && refaddr >= '!' && refaddr <= '~') {
				char ch = refaddr;
				if (ch != ds->chref) {
					ds_begin_comment (ds);
					ds_comment (ds, true, "; '%c'", ch);
				}
			} else if (refaddr > 10) {
				if ((st64)refaddr < 0) {
					// resolve local var if possible
					RAnalVar *v = r_anal_var_get (core->anal, ds->at, 'v', 1, (int)refaddr);
					ds_begin_comment (ds);
					if (v) {
						ds_comment (ds, true, "; var %s", v->name);
						r_anal_var_free (v);
					} else {
						ds_comment (ds, true, "; var %d", -(int)refaddr);
					}
				} else {
					if (r_core_anal_address (core, refaddr) & R_ANAL_ADDR_TYPE_ASCII) {
						if (!string_printed && *msg) {
							ds_print_str (ds, msg, len, refaddr);
							string_printed = true;
						}
					}
				}
			}
			//XXX this should be refactored with along the above
			kind = r_anal_data_kind (core->anal, refaddr, (const ut8*)msg, len - 1);
			if (kind) {
				if (!strcmp (kind, "text")) {
					if (!string_printed && *msg) {
						ds_print_str (ds, msg, len, refaddr);
						string_printed = true;
					}
				} else if (!strcmp (kind, "invalid")) {
					int *n = (int*)&refaddr;
					ut64 p = ds->analop.val;
					if (p == UT64_MAX || p == UT32_MAX) {
						p = ds->analop.ptr;
					}
					/* avoid double ; -1 */
					if (p != UT64_MAX && p != UT32_MAX) {
						if (*n > -0xfff && *n < 0xfff) {
							if (!aligned) {
								ds_begin_comment (ds);
							}
							ds_comment (ds, true, "; %"PFMT64d, p);
						}
					}
				} else {
					// r_cons_printf (" ; %s", kind);
				}
				// TODO: check for more data kinds
			}
		}
		free (msg);
	} else {
		ds_print_as_string (ds);
	}
#if 0
	if (!ds->show_comment_right && ds->cmtcount > 0) {
		ds_newline (ds);
	}
#endif
#if DEADCODE
	if (aligned && ds->show_color) {
		r_cons_printf (Color_RESET);
	}
#endif
}

struct getreloc_t {
	ut64 vaddr;
	int size;
};

static int getreloc_tree(const void *user, const RBNode *n) {
	struct getreloc_t *gr = (struct getreloc_t *)user;
	const RBinReloc *r = container_of (n, const RBinReloc, vrb);
	if ((r->vaddr >= gr->vaddr) && (r->vaddr < (gr->vaddr + gr->size))) {
		return 0;
	}

	if (gr->vaddr > r->vaddr) {
		return 1;
	}
	if (gr->vaddr < r->vaddr) {
		return -1;
	}
	return 0;
}

// TODO: Use sdb in rbin to accelerate this
// we shuold use aligned reloc addresses instead of iterating all of them
static RBinReloc *getreloc(RCore *core, ut64 addr, int size) {
	if (size < 1 || addr == UT64_MAX) {
		return NULL;
	}
	RBNode *relocs = r_bin_get_relocs (core->bin);
	if (!relocs) {
		return NULL;
	}
	struct getreloc_t gr = { .vaddr = addr, .size = size };
	RBNode *res = r_rbtree_find (relocs, &gr, getreloc_tree);
	return res? container_of (res, RBinReloc, vrb): NULL;
}

static void ds_print_relocs(RDisasmState *ds) {
	char *demname = NULL;
	if (!ds->showrelocs || !ds->show_slow) {
		return;
	}
	RCore *core = ds->core;
	const char *lang = r_config_get (core->config, "bin.lang");
	bool demangle = r_config_get_i (core->config, "asm.demangle");
	RBinReloc *rel = getreloc (core, ds->at, ds->analop.size);
	if (rel) {
		int cstrlen = 0;
		char *ll = r_cons_lastline (&cstrlen);
		int ansilen = r_str_ansi_len (ll);
		int utf8len = r_utf8_strlen ((const ut8*)ll);
		int cells = utf8len - (cstrlen - ansilen);
		int len = ds->cmtcol - cells;
		r_cons_memset (' ', len);
		if (rel->import) {
			if (demangle) {
				demname = r_bin_demangle (core->bin->cur, lang, rel->import->name, rel->vaddr);
			}
			r_cons_printf ("; RELOC %d %s", rel->type, demname ? demname : rel->import->name);
		} else if (rel->symbol) {
			if (demangle) {
				demname = r_bin_demangle (core->bin->cur, lang, rel->symbol->name, rel->symbol->vaddr);
			}
			r_cons_printf ("; RELOC %d %s @ 0x%08" PFMT64x " + 0x%" PFMT64x,
					rel->type, demname ? demname : rel->symbol->name,
					rel->symbol->vaddr, rel->addend);
		} else {
			r_cons_printf ("; RELOC %d ", rel->type);
		}
		free (demname);
	}
}

static int mymemwrite0(RAnalEsil *esil, ut64 addr, const ut8 *buf, int len) {
	return 0;
}

static int mymemwrite1(RAnalEsil *esil, ut64 addr, const ut8 *buf, int len) {
	return 1;
}

static int mymemwrite2(RAnalEsil *esil, ut64 addr, const ut8 *buf, int len) {
	return (addr >= emustack_min && addr < emustack_max);
}

static char *ssa_get(RAnalEsil *esil, const char *reg) {
	RDisasmState *ds = esil->user;
	if (isdigit (*reg)) {
		return strdup (reg);
	}
	if (!ds->ssa) {
		ds->ssa = sdb_new0 ();
	}
	int n = sdb_num_get (ds->ssa, reg, NULL);
	return r_str_newf ("%s_%d", reg, n);
}

static void ssa_set(RAnalEsil *esil, const char *reg) {
	RDisasmState *ds = esil->user;
	(void)sdb_num_inc (ds->ssa, reg, 1, 0);
}

#define R_DISASM_MAX_STR 512
static int myregread(RAnalEsil *esil, const char *name, ut64 *res, int *size) {
	RDisasmState *ds = esil->user;
	if (ds && ds->show_emu_ssa) {
		if (!isdigit (*name)) {
			char *r = ssa_get (esil, name);
			ds_comment_esil (ds, true, false, "<%s", r);
			free (r);
		}
	}
	return 0;
}

static int myregwrite(RAnalEsil *esil, const char *name, ut64 *val) {
	char str[64], *msg = NULL;
	ut32 *n32 = (ut32*)str;
	RDisasmState *ds = esil->user;
	if (!ds) {
		return 0;
	}
	if (!ds->show_emu_strlea && ds->analop.type == R_ANAL_OP_TYPE_LEA) {
		// useful for ARM64
		// reduce false positives in emu.str=true when loading strings via adrp+add
		return 0;
	}
	ds->esil_likely = true;
	if (ds->show_emu_ssa) {
		ssa_set (esil, name);
		char *r = ssa_get (esil, name);
		ds_comment_esil (ds, true, false, ">%s", r);
		free (r);
		return 0;
	}
	if (!ds->show_slow) {
		return 0;
	}
	memset (str, 0, sizeof (str));
	if (*val) {
		bool emu_str_printed = false;
		char *type = NULL;
		(void)r_io_read_at (esil->anal->iob.io, *val, (ut8*)str, sizeof (str)-1);
		str[sizeof (str)-1] = 0;
		// support cstring here
		{
			ut64 *cstr = (ut64*) str;
			ut64 addr = cstr[0];
			if (!(*val >> 32)) {
				addr = addr & UT32_MAX;
			}
			if (cstr[0] == 0 && cstr[1] < 0x1000) {
				ut64 addr = cstr[2];
				if (!(*val >> 32)) {
					addr = addr & UT32_MAX;
				}
				(void)r_io_read_at (esil->anal->iob.io, addr,
					(ut8*)str, sizeof (str)-1);
			//	eprintf ("IS CSTRING 0x%llx %s\n", addr, str);
				type = r_str_newf ("(cstr 0x%08"PFMT64x") ", addr);
				ds->printed_str_addr = cstr[2];
			} else if (r_io_is_valid_offset (esil->anal->iob.io, addr, 0)) {
				ds->printed_str_addr = cstr[0];
				type = r_str_newf ("(pstr 0x%08"PFMT64x") ", addr);
				(void)r_io_read_at (esil->anal->iob.io, addr,
					(ut8*)str, sizeof (str)-1);
			//	eprintf ("IS PSTRING 0x%llx %s\n", addr, str);
			}
		}

		if (*str && !r_bin_strpurge (ds->core->bin, str, *val) && r_str_is_printable_incl_newlines (str)
		    && (ds->printed_str_addr == UT64_MAX || *val != ds->printed_str_addr)) {
			bool jump_op = false;
			bool ignored = false;
			switch (ds->analop.type) {
			case R_ANAL_OP_TYPE_JMP:
			case R_ANAL_OP_TYPE_UJMP:
			case R_ANAL_OP_TYPE_RJMP:
			case R_ANAL_OP_TYPE_IJMP:
			case R_ANAL_OP_TYPE_IRJMP:
			case R_ANAL_OP_TYPE_CJMP:
			case R_ANAL_OP_TYPE_MJMP:
			case R_ANAL_OP_TYPE_UCJMP:
				jump_op = true;
				break;
			case R_ANAL_OP_TYPE_TRAP:
			case R_ANAL_OP_TYPE_RET:
				ignored = true;
				break;
			}
			if (!jump_op && !ignored) {
				const char *prefix;
				ut32 len = sizeof (str) -1;
#if 0
				RCore *core = ds->core;
				ut32 len = core->blocksize + 256;
				if (len < core->blocksize || len > R_DISASM_MAX_STR) {
					len = R_DISASM_MAX_STR;
				}
#endif
				char *escstr = ds_esc_str (ds, str, (int)len, &prefix, false);
				if (escstr) {
					char *m;
					if (ds->show_color) {
						bool inv = ds->show_emu_strinv;
						m = r_str_newf ("%s%s%s\"%s\"%s",
						                  prefix, type ? type : "", inv ? Color_INVERT : "",
						                  escstr, inv ? Color_INVERT_RESET : "");
					} else {
						m = r_str_newf ("%s%s\"%s\"", prefix, type? type: "", escstr);
					}
					msg = r_str_append_owned (msg, m);
					emu_str_printed = true;
					free (escstr);
				}
			}
		} else {
			if (!*n32) {
				// msg = strdup ("NULL");
			} else if (*n32 == UT32_MAX) {
				/* nothing */
			} else {
				if (!ds->show_emu_str) {
					msg = r_str_appendf (msg, "-> 0x%x", *n32);
				}
			}
		}
		R_FREE (type);
		if ((ds->printed_flag_addr == UT64_MAX || *val != ds->printed_flag_addr)
		    && (ds->show_emu_strflag || !emu_str_printed)) {
			RFlagItem *fi = r_flag_get_i (esil->anal->flb.f, *val);
			if (fi && (!ds->opstr || !strstr (ds->opstr, fi->name))) {
				msg = r_str_appendf (msg, "%s%s", msg && *msg ? " " : "", fi->name);
			}
		}
	}
	if (ds->show_emu_str) {
		if (msg && *msg) {
			if (ds->show_emu_stroff && *msg == '"') {
				ds_comment_esil (ds, true, false, "; 0x%"PFMT64x" %s", *val, msg);
			} else {
				ds_comment_esil (ds, true, false, "; %s", msg);
			}
			if (ds->show_comments && !ds->show_comment_right) {
				ds_newline (ds);
			}
		}
	} else {
		if (msg && *msg) {
			ds_comment_esil (ds, true, false, "; %s=0x%"PFMT64x" %s", name, *val, msg);
		} else {
			ds_comment_esil (ds, true, false, "; %s=0x%"PFMT64x, name, *val);
		}
		if (ds->show_comments && !ds->show_comment_right) {
			ds_newline (ds);
		}
	}
	free (msg);
	return 0;
}

static void ds_pre_emulation(RDisasmState *ds) {
	bool do_esil = ds->show_emu;
	if (!ds->pre_emu) {
		return;
	}
	RFlagItem *f = r_flag_get_at (ds->core->flags, ds->core->offset, true);
	if (!f) {
		return;
	}
	ut64 base = f->offset;
	RAnalEsil *esil = ds->core->anal->esil;
	int i, end = ds->core->offset - base;
	int maxemu = 1024 * 1024;
	RAnalEsilHookRegWriteCB orig_cb = esil->cb.hook_reg_write;
	if (end < 0 || end > maxemu) {
		return;
	}
	ds->stackptr = ds->core->anal->stackptr;
	esil->cb.hook_reg_write = NULL;
	for (i = 0; i < end; i++) {
		ut64 addr = base + i;
		RAnalOp* op = r_core_anal_op (ds->core, addr, R_ANAL_OP_MASK_ESIL | R_ANAL_OP_MASK_HINT);
		if (op) {
			if (do_esil) {
				r_anal_esil_set_pc (esil, addr);
				r_anal_esil_parse (esil, R_STRBUF_SAFEGET (&op->esil));
				if (op->size > 0) {
					i += op->size - 1;
				}
			}
			ds_update_stackptr (ds, op);
			r_anal_op_free (op);
		}
	}
	esil->cb.hook_reg_write = orig_cb;
}

static void ds_print_esil_anal_init(RDisasmState *ds) {
	RCore *core = ds->core;
	const char *pc = r_reg_get_name (core->anal->reg, R_REG_NAME_PC);
	ds->esil_old_pc = r_reg_getv (core->anal->reg, pc);
	if (!ds->esil_old_pc || ds->esil_old_pc == UT64_MAX) {
		ds->esil_old_pc = core->offset;
	}
	if (!ds->show_emu) {
		// XXX. stackptr not computed without asm.emu, when its not required
		return;
	}
	if (!core->anal->esil) {
		int iotrap = r_config_get_i (core->config, "esil.iotrap");
		int esd = r_config_get_i (core->config, "esil.stack.depth");
		unsigned int addrsize = r_config_get_i (core->config, "esil.addr.size");

		if (!(core->anal->esil = r_anal_esil_new (esd, iotrap, addrsize))) {
			R_FREE (ds->esil_regstate);
			return;
		}
		r_anal_esil_setup (core->anal->esil, core->anal, 0, 0, 1);
	}
	core->anal->esil->user = ds;
	free (ds->esil_regstate);
	R_FREE (core->anal->last_disasm_reg);
	if (core->anal->gp) {
		r_reg_setv (core->anal->reg, "gp", core->anal->gp);
	}
	ds->esil_regstate = r_reg_arena_peek (core->anal->reg);

	// TODO: emulate N instructions BEFORE the current offset to get proper full function emulation
	ds_pre_emulation (ds);
}

static void ds_print_bbline(RDisasmState *ds, bool force) {
	if (ds->show_bbline) {
		RAnalBlock *bb = r_anal_fcn_bbget_at (ds->fcn, ds->at);
		if (force || (ds->fcn && bb)) {
			ds_begin_line (ds);
			ds_setup_print_pre (ds, false, false);
			if (!ds->linesright && ds->show_lines_bb && ds->line) {
				char *refline;
				if (force) { // bbline is after disasm
					refline = r_anal_reflines_str (ds->core, ds->at,
						ds->linesopts | R_ANAL_REFLINE_TYPE_MIDDLE_AFTER);
				} else {
					ds_update_ref_lines (ds);
					refline = ds->refline2;
				}
				r_cons_printf ("%s%s%s", COLOR (ds, color_flow),
						refline, COLOR_RESET (ds));
				if (force) {
					free (refline);
				}
			}
			r_cons_printf ("|");
			ds_newline (ds);
		}
	}
}

static void print_fcn_arg(RCore *core, const char *type, const char *name,
			   const char *fmt, const ut64 addr,
			   const int on_stack, int asm_types) {
	if (on_stack == 1 && asm_types > 1) {
		r_cons_printf ("%s", type);
	}
	if (addr != UT32_MAX && addr != UT64_MAX  && addr != 0) {
		char *res = r_core_cmd_strf (core, "pf%s %s%s %s @ 0x%08" PFMT64x,
				(asm_types==2)? "": "q", (on_stack == 1) ? "*" : "", fmt, name, addr);
		r_str_trim (res);
		r_cons_printf ("%s", res);
		free (res);
	} else {
		r_cons_printf ("-1");
	}
	r_cons_chop ();
}

static void delete_last_comment(RDisasmState *ds) {
	if (!ds->show_comment_right_default) {
		return;
	}
	const char *ll = r_cons_get_buffer ();
	if (!ll) {
		return;
	}
	ll += ds->buf_line_begin;
	const char *begin = ll;
	if (begin) {
		ds_newline (ds);
		ds_begin_line (ds);
		ds_setup_print_pre (ds, false, false);
		if (!ds->linesright && ds->show_lines_bb && ds->line) {
			char *refline = r_anal_reflines_str (ds->core, ds->at,
			                    ds->linesopts | R_ANAL_REFLINE_TYPE_MIDDLE_AFTER);
			r_return_if_fail (refline);
			r_cons_printf ("%s%s%s", COLOR (ds, color_flow),
			               refline, COLOR_RESET (ds));
			free (refline);
		}
	}
}

static bool can_emulate_metadata(RCore * core, ut64 at) {
	const char *infos;
	const char *emuskipmeta = r_config_get (core->config, "emu.skip");
	char key[32];
	Sdb *s = core->anal->sdb_meta;
	snprintf (key, sizeof (key)-1, "meta.0x%"PFMT64x, at);
	infos = sdb_const_get (s, key, 0);
	if (!infos) {
		/* no metadata: let's emulate this */
		return true;
	}
	for (; *infos; infos++) {
		/*
		 * don't emulate if at least one metadata type
		 * can't be emulated
		 */
		if (*infos != ',' && strchr(emuskipmeta, *infos)) {
			return false;
		}
	}
	return true;
}

static void mipsTweak(RDisasmState *ds) {
	RCore *core = ds->core;
	//const char *asm_arch = r_config_get (core->config, "asm.arch");
	//if (asm_arch && *asm_arch && strstr (asm_arch, "mips")) {
		ut64 gp = r_config_get_i (core->config, "anal.gp2");
		if (gp && gp !=UT64_MAX) {
			r_reg_setv (core->anal->reg, "gp", gp);
		}
	//}
}

// modifies anal register state
static void ds_print_esil_anal(RDisasmState *ds) {
	RCore *core = ds->core;
	RAnalEsil *esil = core->anal->esil;
	const char *pc;
	int (*hook_mem_write)(RAnalEsil *esil, ut64 addr, const ut8 *buf, int len) = NULL;
	int i, nargs;
	ut64 at = r_core_pava (core, ds->at);
	RConfigHold *hc = r_config_hold_new (core->config);
	if (!hc) {
		return;
	}
	if (!esil) {
		ds_print_esil_anal_init (ds);
		esil = core->anal->esil;
	}
	if (!ds->show_emu) {
		goto beach;
	}
	if (!can_emulate_metadata (core, at)) {
		goto beach;
	}
	if (ds->show_color) {
		r_cons_strcat (ds->pal_comment);
	}
	esil = core->anal->esil;
	pc = r_reg_get_name (core->anal->reg, R_REG_NAME_PC);
	if (pc) {
		r_reg_setv (core->anal->reg, pc, at + ds->analop.size);
		esil->cb.user = ds;
		esil->cb.hook_reg_write = myregwrite;
		esil->cb.hook_reg_read = myregread;
		hook_mem_write = esil->cb.hook_mem_write;
	}
	if (ds->show_emu_stack) {
		esil->cb.hook_mem_write = mymemwrite2;
	} else {
		if (ds->show_emu_write) {
			esil->cb.hook_mem_write = mymemwrite0;
		} else {
			esil->cb.hook_mem_write = mymemwrite1;
		}
	}
	ds->esil_likely = 0;
	mipsTweak (ds);
	r_anal_esil_set_pc (esil, at);
	r_anal_esil_parse (esil, R_STRBUF_SAFEGET (&ds->analop.esil));
	r_anal_esil_stack_free (esil);
	r_config_hold_i (hc, "io.cache", NULL);
	r_config_set (core->config, "io.cache", "true");
	if (!ds->show_comments) {
		goto beach;
	}
	switch (ds->analop.type) {
	case R_ANAL_OP_TYPE_SWI: {
		char *s = cmd_syscall_dostr (core, ds->analop.val, at);
		if (s) {
			ds_comment_esil (ds, true, true, "; %s", s);
			free (s);
		}
		} break;
	case R_ANAL_OP_TYPE_CJMP:
		ds_comment_esil (ds, true, true, ds->esil_likely? "; likely" : "; unlikely");
		break;
	case R_ANAL_OP_TYPE_JMP:
		{
			ut64 addr = ds->analop.jump;
			if (!r_anal_get_fcn_at (ds->core->anal, addr, R_ANAL_FCN_TYPE_NULL)
					&& !r_flag_get_at (core->flags, addr, false)) {
				break;
			}
		}
	case R_ANAL_OP_TYPE_UCALL:
	case R_ANAL_OP_TYPE_ICALL:
	case R_ANAL_OP_TYPE_RCALL:
	case R_ANAL_OP_TYPE_IRCALL:
	case R_ANAL_OP_TYPE_CALL:
		{
			RAnalFunction *fcn;
			RAnalFuncArg *arg;
			RListIter *iter;
			RListIter *nextele;
			const char *fcn_name = NULL;
			char *key = NULL;
			ut64 pcv = ds->analop.jump;
			if (pcv == UT64_MAX) {
				pcv = ds->analop.ptr; // call [reloc-addr] // windows style
				if (pcv == UT64_MAX || !pcv) {
					r_anal_esil_reg_read (esil, "$jt", &pcv, NULL);
					if (pcv == UT64_MAX || !pcv) {
						pcv = r_reg_getv (core->anal->reg, pc);
					}
				}
			}
			fcn = r_anal_get_fcn_at (core->anal, pcv, 0);
			if (fcn) {
				fcn_name = fcn->name;
			} else {
				RFlagItem *item = r_flag_get_i (core->flags, pcv);
				if (item) {
					fcn_name = item->name;
				}
			}
			if (fcn_name) {
				key = resolve_fcn_name (core->anal, fcn_name);
			}
			if (key) {
				if (ds->asm_types < 1) {
					break;
				}
				const char *fcn_type = r_type_func_ret (core->anal->sdb_types, key);
				int nargs = r_type_func_args_count (core->anal->sdb_types, key);
				// remove other comments
				delete_last_comment (ds);
				// ds_comment_start (ds, "");
				ds_comment_esil (ds, true, false, "%s", ds->show_color ? ds->pal_comment : "");
				if (fcn_type) {
					ds_comment_middle (ds, "; %s%s%s(", r_str_get (fcn_type),
							(*fcn_type && fcn_type[strlen (fcn_type) - 1] == '*') ? "" : " ",
							r_str_get (key));
					if (!nargs) {
						ds_comment_end (ds, "void)");
						break;
					}
				}
			}
			ut64 s_width = (core->anal->bits == 64)? 8: 4;
			const char *sp = r_reg_get_name (core->anal->reg, R_REG_NAME_SP);
			ut64 spv = r_reg_getv (core->anal->reg, sp);
			r_reg_setv (core->anal->reg, sp, spv + s_width); // temporarily set stack ptr to sync with carg.c
			RList *list = r_core_get_func_args (core, fcn_name);
			if (!r_list_empty (list)) {
				bool warning = false;
				bool on_stack = false;
				r_list_foreach (list, iter, arg) {
					if (r_str_startswith (arg->cc_source, "stack")) {
						on_stack = true;
					}
					if (!arg->size) {
						if (ds->asm_types == 2) {
							ds_comment_middle (ds, "%s: unk_size", arg->c_type);
						}
						warning = true;
					}
					nextele = r_list_iter_get_next (iter);
					if (!arg->fmt) {
						if (ds->asm_types > 1) {
							if (warning) {
								ds_comment_middle (ds, "_format");
							} else {
								ds_comment_middle (ds, "%s : unk_format", arg->c_type);
							}
						} else {
							ds_comment_middle (ds, "?");
						}
						ds_comment_middle (ds, nextele?", ":")");
					} else {
						// TODO: may need ds_comment_esil
						print_fcn_arg (core, arg->orig_c_type, arg->name, arg->fmt, arg->src, on_stack, ds->asm_types);
						ds_comment_middle (ds, nextele?", ":")");
					}
				}
				ds_comment_end (ds, "");
				break;
			} else {
				// function name not resolved
				nargs = DEFAULT_NARGS;
				if (fcn) {
					// @TODO: fcn->nargs should be updated somewhere and used here instead
					nargs = r_anal_var_count (core->anal, fcn, 's', 1) +
							r_anal_var_count (core->anal, fcn, 'b', 1) +
							r_anal_var_count (core->anal, fcn, 'r', 1);
				}
				if (nargs > 0) {
					if (fcn_name) {
						ds_comment_middle (ds, "; %s(", fcn_name);
					} else {
						ds_comment_middle (ds, "; 0x%"PFMT64x"(", pcv);
					}
					for (i = 0; i < nargs; i++) {
						ut64 v = r_debug_arg_get (core->dbg, R_ANAL_CC_TYPE_FASTCALL, i);
						ds_comment_middle (ds, "%s0x%"PFMT64x, i?", ":"", v);
					}
					ds_comment_end (ds, ")");
				}
			}
			r_reg_setv (core->anal->reg, sp, spv); // reset stack ptr
		}
		break;
	}
	ds_print_color_reset (ds);
beach:
	if (esil) {
		esil->cb.hook_mem_write = hook_mem_write;
	}
	r_config_hold_restore (hc);
	r_config_hold_free (hc);
}

static void ds_print_calls_hints(RDisasmState *ds) {
	int emu = r_config_get_i (ds->core->config, "asm.emu");
	int emuwrite = r_config_get_i (ds->core->config, "emu.write");
	if (emu && emuwrite) {
		// this is done by ESIL
		return;
	}
	RAnal *anal = ds->core->anal;
	// RAnalFunction *fcn = r_anal_get_fcn_in (anal, ds->analop.jump, -1);
	RAnalFunction *fcn = fcnIn (ds, ds->analop.jump, -1);
	Sdb *TDB = anal->sdb_types;
	char *name;
	if (!fcn) {
		return;
	}
	if (r_type_func_exist (TDB, fcn->name)) {
		name = strdup (fcn->name);
	} else if (!(name = r_type_func_guess (TDB, fcn->name))) {
		return;
	}
	ds_begin_comment (ds);
	const char *fcn_type = r_type_func_ret (TDB, name);
	if (!fcn_type || !*fcn_type) {
		free (name);
		return;
	}
	char *cmt = r_str_newf ("; %s%s%s(", fcn_type,
		fcn_type[strlen (fcn_type) - 1] == '*' ? "" : " ",
		name);
	int i, arg_max = r_type_func_args_count (TDB, name);
	if (!arg_max) {
		cmt = r_str_append (cmt, "void)");
	} else {
		for (i = 0; i < arg_max; i++) {
			char *type = r_type_func_args_type (TDB, name, i);
			const char *tname = r_type_func_args_name (TDB, name, i);
			if (type && *type) {
				cmt = r_str_appendf (cmt, "%s%s%s%s%s", i == 0 ? "": " ", type,
						type[strlen (type) -1] == '*' ? "": " ",
						tname, i == arg_max - 1 ? ")": ",");
			} else if (tname && !strcmp (tname, "...")) {
				cmt = r_str_appendf (cmt, "%s%s%s", i == 0 ? "": " ",
						tname, i == arg_max - 1 ? ")": ",");
			}
			free (type);
		}
	}
	ds_comment (ds, true, cmt);
	ds_print_color_reset (ds);
	free (cmt);
	free (name);
}

static void ds_print_comments_right(RDisasmState *ds) {
	char *desc = NULL;
	RCore *core = ds->core;
	ds_print_relocs (ds);
	if (ds->asm_describe && !ds->has_description) {
		char *op, *locase = strdup (r_asm_op_get_asm (&ds->asmop));
		if (!locase) {
			return;
		}
		op = strchr (locase, ' ');
		if (op) {
			*op = 0;
		}
		r_str_case (locase, 0);
		desc = r_asm_describe (core->assembler, locase);
		free (locase);
	}
	if (ds->show_usercomments || ds->show_comments) {
		if (desc && *desc) {
			ds_align_comment (ds);
			if (ds->show_color) {
				r_cons_strcat (ds->color_comment);
			}
			r_cons_strcat ("; ");
			r_cons_strcat (desc);
			ds_print_color_reset (ds);
		}
		if (ds->show_comment_right && ds->comment) {
			char *comment = r_str_trim (ds->comment);
			if (*comment) {
				if (!desc) {
					ds_align_comment (ds);
				}
				if (strchr (comment, '\n')) {
					comment = strdup (comment);
					if (comment) {
						ds_newline (ds);
						ds_begin_line (ds);
						int lines_count;
						int *line_indexes = r_str_split_lines (comment, &lines_count);
						if (line_indexes) {
							int i;
							for (i = 0; i < lines_count; i++) {
								char *c = comment + line_indexes[i];
								ds_print_pre (ds);
								if (ds->show_color) {
									r_cons_strcat (ds->color_usrcmt);
								}
								r_cons_printf (i == 0 ? "%s" : "; %s", c);
								if (i < lines_count - 1) {
									ds_newline (ds);
									ds_begin_line (ds);
								}
							}
						}
						free (line_indexes);
					}
					free (comment);
				} else {
					if (comment) {
						r_cons_strcat (comment);
					}
				}
			}
			//r_cons_strcat_justify (comment, strlen (ds->refline) + 5, ';');
			ds_print_color_reset (ds);
			R_FREE (ds->comment);
		}
	}
	free (desc);
	if (ds->analop.type == R_ANAL_OP_TYPE_CALL && ds->show_calls) {
		ds_print_calls_hints (ds);
	}
}

static void ds_print_as_string(RDisasmState *ds) {
	char *str = r_num_as_string (NULL, ds->analop.ptr, true);
	if (str) {
		ds_comment (ds, false, "%s; \"%s\"%s", COLOR (ds, pal_comment),
			str, COLOR_RESET (ds));
	}
	free (str);
}

static char *_find_next_number(char *op) {
	char *p = op;
	if (p) {
		while (*p) {
			// look for start of next separator or ANSI sequence
			while (*p && !IS_SEPARATOR (*p) && *p != 0x1b) {
				p++;
			}
			if (*p == 0x1b) {
				// skip to end of ANSI sequence (lower or uppercase char)
				while (*p && !(*p >= 'A' && *p <= 'Z') && !(*p >= 'a' && *p <= 'z')) {
					p++;
				}
				if (*p) {
					p++;
				}
			}
			if (IS_SEPARATOR (*p)) {
				// skip to end of separator
				while (*p && IS_SEPARATOR (*p)) {
					p++;
				}
			}
			if (IS_DIGIT (*p)) {
				// we found the start of the next number
				return p;
			}
		}
	}
	return NULL;
}

static char *ds_sub_jumps(RDisasmState *ds, char *str) {
	RAnal *anal = ds->core->anal;
	RFlag *f = ds->core->flags;
	RFlagItem *flag;
	const char *name = NULL;

	if (!ds->jmpsub || !anal) {
		return str;
	}
	int optype = ds->analop.type & 0xFFFF;
	if (optype < R_ANAL_OP_TYPE_JMP || optype >= R_ANAL_OP_TYPE_RET) {
		return str;
	}
	ut64 addr = ds->analop.jump;

	RAnalFunction *fcn = r_anal_get_fcn_at (anal, addr, 0);
	if (fcn) {
		name = fcn->name;
	} else if (f) {
		flag = r_flag_get_i2 (f, addr);
		if (flag) {
			name = flag->name;
		} else {
			RBinReloc *rel = getreloc (ds->core, addr, ds->analop.size);
			if (rel && rel->import && rel->import->name) {
				name = rel->import->name;
			}
		}
	}
	if (name) {
		char *nptr, *ptr;
		ut64 numval;
		ptr = str;
		while ((nptr = _find_next_number (ptr))) {
			ptr = nptr;
			numval = r_num_get (NULL, ptr);
			if (numval == addr) {
				while (*nptr && !IS_SEPARATOR (*nptr) && *nptr != 0x1b) {
					nptr++;
				}
				char* numstr = r_str_ndup (ptr, nptr-ptr);
				if (numstr) {
					str = r_str_replace (str, numstr, name, 0);
					free (numstr);
				}
				break;
			}
		}
	}
	return str;
}

static bool line_highlighted(RDisasmState *ds) {
	return ds->asm_highlight != UT64_MAX && ds->vat == ds->asm_highlight;
}

static void ds_start_line_highlight(RDisasmState *ds) {
	if (ds->show_color && line_highlighted (ds)) {
		r_cons_strcat (ds->color_linehl);
	}
}

static void ds_end_line_highlight(RDisasmState *ds) {
	if (ds->show_color && line_highlighted (ds)) {
		r_cons_strcat (Color_RESET);
	}
}

// int l is for lines
R_API int r_core_print_disasm(RPrint *p, RCore *core, ut64 addr, ut8 *buf, int len, int l, int invbreak, int cbytes, bool json, PJ *pj, RAnalFunction *pdf) {
	int continueoninvbreak = (len == l) && invbreak;
	RAnalFunction *of = NULL;
	RAnalFunction *f = NULL;
	bool calc_row_offsets = p->calc_row_offsets;
	int ret, i, inc, skip_bytes_flag = 0, skip_bytes_bb = 0, idx = 0;
	ut8 *nbuf = NULL;
	const int addrbytes = core->io->addrbytes;

	// TODO: All those ds must be print flags
	RDisasmState *ds = ds_init (core);
	ds->cbytes = cbytes;
	ds->print = p;
	ds->l = l;
	ds->buf = buf;
	ds->len = len;
	ds->addr = addr;
	ds->hint = NULL;
	ds->buf_line_begin = 0;
	ds->pdf = pdf;

	if (json) {
		ds->pj = pj ? pj : pj_new ();
		if (!ds->pj) {
			return 0;
		}
		r_cons_push ();
	} else {
		ds->pj = NULL;
	}

	// disable row_offsets to prevent other commands to overwrite computed info
	p->calc_row_offsets = false;

	//r_cons_printf ("len =%d l=%d ib=%d limit=%d\n", len, l, invbreak, p->limit);
	// TODO: import values from debugger is possible
	// TODO: allow to get those register snapshots from traces
	// TODO: per-function register state trace
	// XXX - is there a better way to reset a the analysis counter so that
	// when code is disassembled, it can actually find the correct offsets
	{ /* used by asm.emu */
		r_reg_arena_push (core->anal->reg);
	}
	if (core->anal->cur && core->anal->cur->reset_counter) {
		core->anal->cur->reset_counter (core->anal, addr);
	}

	ds_reflines_init (ds);
	core->inc = 0;
	/* reset jmp table if not asked to keep it */
	if (!core->keep_asmqjmps) { // hack
		core->asmqjmps_count = 0;
		ut64 *p = realloc (core->asmqjmps, R_CORE_ASMQJMPS_NUM * sizeof (ut64));
		if (p) {
			core->asmqjmps_size = R_CORE_ASMQJMPS_NUM;
			core->asmqjmps = p;
			for (i = 0; i < R_CORE_ASMQJMPS_NUM; i++) {
				core->asmqjmps[i] = UT64_MAX;
			}
		}
	}
	if (ds->pj && !pj) {
		pj_a (ds->pj);
	}
toro:
	// uhm... is this necesary? imho can be removed
	r_asm_set_pc (core->assembler, r_core_pava (core, ds->addr + idx));
	core->cons->vline = r_config_get_i (core->config, "scr.utf8") ? (r_config_get_i (core->config, "scr.utf8.curvy") ? r_vline_uc : r_vline_u) : r_vline_a;

	if (core->print->cur_enabled) {
		// TODO: support in-the-middle-of-instruction too
		r_anal_op_fini (&ds->analop);
		if (r_anal_op (core->anal, &ds->analop, core->offset + core->print->cur,
			buf + core->print->cur, (int)(len - core->print->cur), R_ANAL_OP_MASK_ALL)) {
			// TODO: check for ds->analop.type and ret
			ds->dest = ds->analop.jump;
		}
	} else {
		/* highlight eip */
		const char *pc = core->anal->reg->name[R_REG_NAME_PC];
		if (pc) {
			RFlagItem *item = r_flag_get (core->flags, pc);
			if (item) {
				ds->dest = item->offset;
			}
		}
	}

	ds_print_esil_anal_init (ds);
	inc = 0;
	if (!ds->l) {
		ds->l = core->blocksize;
	}
	r_anal_merge_hint_ranges (core->anal);
	r_cons_break_push (NULL, NULL);
	for (i = idx = ret = 0; addrbytes * idx < len && ds->lines < ds->l; idx += inc, i++, ds->index += inc, ds->lines++) {
		ds->at = ds->addr + idx;
		ds->vat = r_core_pava (core, ds->at);
		if (r_cons_is_breaked ()) {
			R_FREE (nbuf);
			if (ds->pj) {
				r_cons_pop ();
			}
			r_cons_break_pop ();
			ds_free (ds);
			return 0; //break;
		}
		if (core->print->flags & R_PRINT_FLAGS_UNALLOC) {
			if (!core->anal->iob.is_valid_offset (core->anal->iob.io, ds->at, 0)) {
				ds_begin_line (ds);
				ds_print_labels (ds, f);
				ds_setup_print_pre (ds, false, false);
				ds_print_lines_left (ds);
				core->print->resetbg = (ds->asm_highlight == UT64_MAX);
				ds_start_line_highlight (ds);
				ds_print_offset (ds);
				r_cons_printf ("  unmapped\n");
				inc = 1;
				continue;
			}
		}
		r_core_seek_archbits (core, ds->at); // slow but safe
		ds->has_description = false;
		ds->hint = r_core_hint_begin (core, ds->hint, ds->at);
		ds->printed_str_addr = UT64_MAX;
		ds->printed_flag_addr = UT64_MAX;
		// XXX. this must be done in ds_update_pc()
		// ds_update_pc (ds, ds->at);
		r_asm_set_pc (core->assembler, ds->at);
		ds_update_ref_lines (ds);
		r_anal_op_fini (&ds->analop);
		r_anal_op (core->anal, &ds->analop, ds->at, buf + addrbytes * idx, (int)(len - addrbytes * idx), R_ANAL_OP_MASK_ALL);
		if (ds_must_strip (ds)) {
			inc = ds->analop.size;
			// inc = ds->asmop.payload + (ds->asmop.payload % ds->core->assembler->dataalign);
			r_anal_op_fini (&ds->analop);
			continue;
		}
		// f = r_anal_get_fcn_in (core->anal, ds->at, R_ANAL_FCN_TYPE_NULL);
		f = ds->fcn = fcnIn (ds, ds->at, R_ANAL_FCN_TYPE_NULL);
		if (f && f->folded && r_anal_fcn_is_in_offset (f, ds->at)) {
			int delta = (ds->at <= f->addr)? (ds->at - f->addr + r_anal_fcn_size (f)): 0;
			if (of != f) {
				char cmt[32];
				get_bits_comment (core, f, cmt, sizeof (cmt));
				char *comment = r_meta_get_string (core->anal, R_META_TYPE_COMMENT, ds->at);
				if (comment) {
					ds_pre_xrefs (ds, true);
					r_cons_printf ("; %s\n", comment);
					free (comment);
				}
				r_cons_printf ("%s%s%s (fcn) %s%s%s\n",
					COLOR (ds, color_fline), core->cons->vline[CORNER_TL],
					COLOR (ds, color_fname), f->name, cmt, COLOR_RESET (ds));
				ds_setup_print_pre (ds, true, false);
				ds_print_lines_left (ds);
				ds_print_offset (ds);
				r_cons_printf ("(%d byte folded function)\n", r_anal_fcn_size (f));
				//r_cons_printf ("%s%s%s\n", COLOR (ds, color_fline), core->cons->vline[RDWN_CORNER], COLOR_RESET (ds));
				if (delta < 0) {
					delta = -delta;
				}
				ds->addr += delta + idx;
				r_io_read_at (core->io, ds->addr, buf, len);
				inc = 0; //delta;
				idx = 0;
				of = f;
				r_anal_op_fini (&ds->analop);
				if (len == l) {
					break;
				}
				continue;
			} else {
				ds->lines--;
				ds->addr += 1;
				r_io_read_at (core->io, ds->addr, buf, len);
				inc = 0; //delta;
				idx = 0;
				r_anal_op_fini (&ds->analop);
				continue;
			}
		}
		ds_show_comments_right (ds);
		// TRY adding here
		char *link_key = sdb_fmt ("link.%08"PFMT64x, ds->addr + idx);
		const char *link_type = sdb_const_get (core->anal->sdb_types, link_key, 0);
		if (link_type) {
			char *fmt = r_type_format (core->anal->sdb_types, link_type);
			if (fmt) {
				r_cons_printf ("(%s)\n", link_type);
				r_core_cmdf (core, "pf %s @ 0x%08"PFMT64x"\n", fmt, ds->addr + idx);
				inc += r_type_get_bitsize (core->anal->sdb_types, link_type) / 8;
				free (fmt);
				r_anal_op_fini (&ds->analop);
				continue;
			}
		} else {
			if (idx >= 0) {
				ret = ds_disassemble (ds, buf + addrbytes * idx, len - addrbytes * idx);
				if (ret == -31337) {
					inc = ds->oplen;
					r_anal_op_fini (&ds->analop);
					continue;
				}
			}
		}
		if (ds->retry) {
			ds->retry = false;
			r_cons_break_pop ();
			r_anal_op_fini (&ds->analop);
			goto retry;
		}
		ds_atabs_option (ds);
		// TODO: store previous oplen in core->dec
		if (!core->inc) {
			core->inc = ds->oplen;
		}
		// OOPs. double analysis here?
#if 0
		if (ds->analop.mnemonic || !ds->lastfail) {
			r_anal_op_fini (&ds->analop);
		}
		if (!ds->lastfail) {
			r_anal_op (core->anal, &ds->analop, ds->at, buf + addrbytes * idx, (int)(len - addrbytes * idx));
		}
#else
		if (ds->analop.addr != ds->at) {
			r_anal_op_fini (&ds->analop);
			r_anal_op (core->anal, &ds->analop, ds->at, buf + addrbytes * idx, (int)(len - addrbytes * idx), R_ANAL_OP_MASK_ALL);
		}
#endif
		if (ret < 1) {
			r_strbuf_fini (&ds->analop.esil);
			r_strbuf_init (&ds->analop.esil);
			ds->analop.type = R_ANAL_OP_TYPE_ILL;
		}
		if (ds->hint) {
			if (ds->hint->size) {
				ds->analop.size = ds->hint->size;
			}
			if (ds->hint->ptr) {
				ds->analop.ptr = ds->hint->ptr;
			}
		}
		ds_print_bbline (ds, false);
		if (ds->at >= addr) {
			r_print_set_rowoff (core->print, ds->lines, ds->at - addr, calc_row_offsets);
		}
		if (ds->midflags) {
			skip_bytes_flag = handleMidFlags (core, ds, true);
			if (skip_bytes_flag && ds->midflags == R_MIDFLAGS_SHOW) {
				ds->at += skip_bytes_flag;
			}
		}
		ds_show_flags (ds);
		if (skip_bytes_flag && ds->midflags == R_MIDFLAGS_SHOW) {
			ds->at -= skip_bytes_flag;
		}
		if (ds->midbb) {
			skip_bytes_bb = handleMidBB (core, ds);
		}
		if (ds->pdf) {
			static bool sparse = false;
			RAnalBlock *bb = r_anal_fcn_bbget_in (core->anal, ds->pdf, ds->at);
			if (!bb) {
				for (inc = 1; inc < ds->oplen; inc++) {
					RAnalBlock *bb = r_anal_fcn_bbget_in (core->anal, ds->pdf, ds->at + inc);
					if (bb) {
						break;
					}
				}
				r_anal_op_fini (&ds->analop);
				if (!sparse) {
					r_cons_printf ("..\n");
					sparse = true;
				}
				continue;
			}
			sparse = false;
		}
		ds_control_flow_comments (ds);
		ds_adistrick_comments (ds);
		/* XXX: This is really cpu consuming.. need to be fixed */
		ds_show_functions (ds);
		ds_show_xrefs (ds);

		if (ds->show_comments && !ds->show_comment_right) {
			ds_instruction_mov_lea (ds, idx);
			ds_show_refs (ds);
			ds_build_op_str (ds, false);
			ds_print_ptr (ds, len + 256, idx);
			if (!ds->pseudo) {
				R_FREE (ds->opstr);
			}
			ds_print_sysregs (ds);
			ds_print_fcn_name (ds);
			ds_print_color_reset (ds);
			if (ds->show_emu) {
				ds_print_esil_anal (ds);
			}
			if (ds->analop.type == R_ANAL_OP_TYPE_CALL && ds->show_calls) {
				ds_print_calls_hints (ds);
			}
			/* respect asm.describe */
			char *desc = NULL;
			if (ds->asm_describe && !ds->has_description) {
				char *op, *locase = strdup (r_asm_op_get_asm (&ds->asmop));
				if (!locase) {
					break;
				}
				op = strchr (locase, ' ');
				if (op) {
					*op = 0;
				}
				r_str_case (locase, 0);
				desc = r_asm_describe (core->assembler, locase);
				free (locase);
			}
			if (desc && *desc) {
				ds_begin_comment(ds);
				ds_align_comment (ds);
				if (ds->show_color) {
					r_cons_strcat (ds->color_comment);
				}
				r_cons_strcat ("; ");
				r_cons_strcat (desc);
				ds_print_color_reset (ds);
				ds_newline (ds);
				free (desc);
			}
		}

		ds_begin_line (ds);
		f = fcnIn (ds, ds->addr, 0);
		ds_print_labels (ds, f);
		ds_setup_print_pre (ds, false, false);
		ds_print_lines_left (ds);
		core->print->resetbg = (ds->asm_highlight == UT64_MAX);
		ds_start_line_highlight (ds);
		ds_print_offset (ds);
		if (ds->asm_hint_pos == 0) {
			ds_print_core_vmode (ds, ds->asm_hint_pos);
		}
		ds_print_op_size (ds);
		ds_print_trace (ds);
		ds_print_cycles (ds);
		ds_print_family (ds);
		ds_print_stackptr (ds);
		ret = ds_print_meta_infos (ds, buf, len, idx);
		if (ds->mi_found) {
			ds_print_dwarf (ds);
			ret = ds_print_middle (ds, ret);

			ds_print_asmop_payload (ds, buf + addrbytes * idx);
			if (core->assembler->syntax != R_ASM_SYNTAX_INTEL) {
				RAsmOp ao; /* disassemble for the vm .. */
				int os = core->assembler->syntax;
				r_asm_set_syntax (core->assembler, R_ASM_SYNTAX_INTEL);
				r_asm_disassemble (core->assembler, &ao, buf + addrbytes * idx,
					len - addrbytes * idx + 5);
				r_asm_set_syntax (core->assembler, os);
			}
			if (ds->asm_hint_pos > 0) {
				ds_print_core_vmode (ds, ds->asm_hint_pos);
			}
			ds_end_line_highlight (ds);
			if ((ds->show_comments || ds->show_usercomments) && ds->show_comment_right) {
				ds_print_color_reset (ds);
				ds_print_comments_right (ds);
			}
		} else {
			ds->mi_found = false;
			/* show cursor */
			ds_print_show_cursor (ds);
			ds_print_show_bytes (ds);
			ds_print_lines_right (ds);
			ds_build_op_str (ds, true);
			ds_print_opstr (ds);
			ds_end_line_highlight (ds);
			ds_print_dwarf (ds);
			ret = ds_print_middle (ds, ret);

			ds_print_asmop_payload (ds, buf + addrbytes * idx);
			if (core->assembler->syntax != R_ASM_SYNTAX_INTEL) {
				RAsmOp ao; /* disassemble for the vm .. */
				int os = core->assembler->syntax;
				r_asm_set_syntax (core->assembler, R_ASM_SYNTAX_INTEL);
				r_asm_disassemble (core->assembler, &ao, buf + addrbytes * idx,
					len - addrbytes * idx + 5);
				r_asm_set_syntax (core->assembler, os);
			}
			if (ds->asm_hint_pos > 0) {
				ds_print_core_vmode (ds, ds->asm_hint_pos);
			}
			// ds_print_cc_update (ds);

			ds_cdiv_optimization (ds);
			if ((ds->show_comments || ds->show_usercomments) && ds->show_comment_right) {
				ds_instruction_mov_lea (ds, idx);
				ds_print_ptr (ds, len + 256, idx);
				ds_print_sysregs (ds);
				ds_print_fcn_name (ds);
				ds_print_color_reset (ds);
				ds_print_comments_right (ds);
				ds_print_esil_anal (ds);
				ds_show_refs (ds);
			}
		}
		core->print->resetbg = true;
		ds_newline (ds);
		if (ds->show_bbline && !ds->bblined && !ds->fcn) {
			switch (ds->analop.type) {
			case R_ANAL_OP_TYPE_MJMP:
			case R_ANAL_OP_TYPE_UJMP:
			case R_ANAL_OP_TYPE_IJMP:
			case R_ANAL_OP_TYPE_RJMP:
			case R_ANAL_OP_TYPE_IRJMP:
			case R_ANAL_OP_TYPE_CJMP:
			case R_ANAL_OP_TYPE_JMP:
			case R_ANAL_OP_TYPE_RET:
				ds_print_bbline (ds, true);
				break;
			}
		}
		if (ds->line) {
			if (ds->show_lines_ret && ds->analop.type == R_ANAL_OP_TYPE_RET) {
				if (strchr (ds->line, '>')) {
					memset (ds->line, ' ', r_str_len_utf8 (ds->line));
				}
				ds_begin_line (ds);
				ds_print_pre (ds);
				r_cons_printf ("%s%s%s; --------------------------------------",
					COLOR (ds, color_flow), ds->line, COLOR_RESET (ds));
				ds_newline (ds);
			}
			R_FREE (ds->line);
			R_FREE (ds->refline);
			R_FREE (ds->refline2);
		}
		R_FREE (ds->opstr);
		inc = ds->oplen;

		if (ds->midflags == R_MIDFLAGS_REALIGN && skip_bytes_flag) {
			inc = skip_bytes_flag;
		}
		if (skip_bytes_bb && skip_bytes_bb < inc) {
			inc = skip_bytes_bb;
		}
		if (inc < 1) {
			inc = 1;
		}
		inc += ds->asmop.payload + (ds->asmop.payload % ds->core->assembler->dataalign);
	}
	r_anal_op_fini (&ds->analop);

	R_FREE (nbuf);
	r_cons_break_pop ();

#if HASRETRY
	if (!ds->cbytes && ds->lines < ds->l) {
		ds->addr = ds->at + inc;
	retry:
		if (len < 4) {
			len = 4;
		}
		if (nbuf) {
			free (nbuf);
		}
		buf = nbuf = malloc (len);
		if (ds->tries > 0) {
			if (r_io_read_at (core->io, ds->addr, buf, len)) {
				goto toro;
			}
		}
		if (ds->lines < ds->l) {
			//ds->addr += idx;
			if (!r_io_read_at (core->io, ds->addr, buf, len)) {
				//ds->tries = -1;
			}
			goto toro;
		}
		if (continueoninvbreak) {
			goto toro;
		}
		R_FREE (nbuf);
	}
#endif
	if (ds->pj) {
		r_cons_pop ();
		if (!pj) {
			pj_end (ds->pj);
			r_cons_printf ("%s", pj_string (ds->pj));
			pj_free (ds->pj);
		}
	}
	r_print_set_rowoff (core->print, ds->lines, ds->at - addr, calc_row_offsets);
	r_print_set_rowoff (core->print, ds->lines + 1, UT32_MAX, calc_row_offsets);
	// TODO: this too (must review)
	ds_print_esil_anal_fini (ds);
	ds_reflines_fini (ds);
	ds_free (ds);
	R_FREE (nbuf);
	p->calc_row_offsets = calc_row_offsets;
	/* used by asm.emu */
	r_reg_arena_pop (core->anal->reg);
	return addrbytes * idx; //-ds->lastfail;
}

/* Disassemble either `nb_opcodes` instructions, or
 * `nb_bytes` bytes; both can be negative.
 * Set to 0 the parameter you don't use */
R_API int r_core_print_disasm_instructions(RCore *core, int nb_bytes, int nb_opcodes) {
	RDisasmState *ds = NULL;
	int i, j, ret, len = 0;
	char *tmpopstr;
	const ut64 old_offset = core->offset;
	bool hasanal = false;
	int nbytes = 0;
	const int addrbytes = core->io->addrbytes;
	int skip_bytes_flag = 0, skip_bytes_bb = 0;

	r_reg_arena_push (core->anal->reg);
	if (!nb_bytes) {
		nb_bytes = core->blocksize;
		if (nb_opcodes < 0) {
			/* Backward disassembly or nb_opcodes opcodes
			 * - We compute the new starting offset
			 * - Read at the new offset */
			nb_opcodes = -nb_opcodes;

			// We have some anal_info.
			if (r_core_prevop_addr (core, core->offset, nb_opcodes, &core->offset)) {
				nbytes = old_offset - core->offset;
			} else {
				// core->offset is modified by r_core_prevop_addr
				core->offset = old_offset;
				r_core_asm_bwdis_len (core, &nbytes, &core->offset, nb_opcodes);
			}
			if (nbytes > core->blocksize) {
				r_core_block_size (core, nbytes);
			}
			r_io_read_at (core->io, core->offset, core->block, nbytes);
		}
	} else {
		if (nb_bytes < 0) { // Disassemble backward `nb_bytes` bytes
			nb_bytes = -nb_bytes;
			core->offset -= nb_bytes;
			if (nb_bytes > core->blocksize) {
				ut64 obsz = core->blocksize;
				r_core_block_size (core, nb_bytes);
				if (core->blocksize == nb_bytes) {
					r_io_read_at (core->io, core->offset, core->block, nb_bytes);
				} else {
					eprintf ("Cannot read that much!\n");
					r_core_block_size (core, obsz);
					len = -1;
					goto err_offset;
				}
				r_core_block_size (core, obsz);
			} else {
				r_io_read_at (core->io, core->offset, core->block, nb_bytes);
			}
		} else {
			if (nb_bytes > core->blocksize) {
				r_core_block_size (core, nb_bytes);
				r_io_read_at (core->io, core->offset, core->block, nb_bytes);
			}
		}
	}

	// XXX - is there a better way to reset a the analysis counter so that
	// when code is disassembled, it can actually find the correct offsets
	if (core->anal->cur && core->anal->cur->reset_counter) {
		core->anal->cur->reset_counter (core->anal, core->offset);
	}
	ds = ds_init (core);
	ds->l = nb_opcodes;
	ds->len = nb_opcodes * 8;

	if (ds->len > core->blocksize) {
		if (core->fixedblock) {
			nb_bytes = ds->len = core->blocksize;
		} else {
			r_core_block_size (core, ds->len);
			r_core_block_read (core);
		}
	}
	if (!ds->l) {
		ds->l = ds->len;
	}
	r_anal_merge_hint_ranges (core->anal);
	r_cons_break_push (NULL, NULL);
	//build ranges to map addr with bits
#define isNotTheEnd (nb_opcodes ? j < nb_opcodes: addrbytes * i < nb_bytes)
	for (i = j = 0; isNotTheEnd; i += ret, j++) {
		ds->at = core->offset + i;
		ds->vat = r_core_pava (core, ds->at);
		hasanal = false;
		r_core_seek_archbits (core, ds->at);
		if (r_cons_is_breaked ()) {
			break;
		}
		ds->hint = r_core_hint_begin (core, ds->hint, ds->at);
		ds->has_description = false;
		r_asm_set_pc (core->assembler, ds->at);
		// XXX copypasta from main disassembler function
		// r_anal_get_fcn_in (core->anal, ds->at, R_ANAL_FCN_TYPE_NULL);
		ret = r_asm_disassemble (core->assembler, &ds->asmop,
			core->block + addrbytes * i, core->blocksize - addrbytes * i);
		ds->oplen = ret;
		if (ds->midflags) {
			skip_bytes_flag = handleMidFlags (core, ds, true);
		}
		if (ds->midbb) {
			skip_bytes_bb = handleMidBB (core, ds);
		}
		if (skip_bytes_flag && ds->midflags > R_MIDFLAGS_SHOW) {
			ret = skip_bytes_flag;
		}
		if (skip_bytes_bb && skip_bytes_bb < ret) {
			ret = skip_bytes_bb;
		}
		r_anal_op_fini (&ds->analop);
		if (ds->show_color && !hasanal) {
			r_anal_op (core->anal, &ds->analop, ds->at, core->block + addrbytes * i, core->blocksize - addrbytes * i, R_ANAL_OP_MASK_ALL);
			hasanal = true;
		}
		if (ds_must_strip (ds)) {
			continue;
		}

		// r_conf = s_printf ("0x%08"PFMT64x"  ", core->offset+i);
		if (ds->hint && ds->hint->size) {
			ret = ds->hint->size;
			ds->oplen = ret;
			ds->analop.size = ret;
			ds->asmop.size = ret;
		}
		/* fix infinite loop */
		if (ret < 1) {
			ret = 1;
		}
		len += R_MAX (0, ret);
		if (ds->hint && ds->hint->opcode) {
			free (ds->opstr);
			ds->opstr = strdup (ds->hint->opcode);
		} else {
			if (ds->decode && !ds->immtrim) {
				free (ds->opstr);
				if (!hasanal) {
					r_anal_op (core->anal, &ds->analop, ds->at, core->block+i, core->blocksize-i, R_ANAL_OP_MASK_ALL);
					hasanal = true;
				}
				tmpopstr = r_anal_op_to_string (core->anal, &ds->analop);
				ds->opstr = (tmpopstr)? tmpopstr: strdup (r_asm_op_get_asm (&ds->asmop));
			} else if (ds->immtrim) {
				ds->opstr = strdup (r_asm_op_get_asm (&ds->asmop));
				r_parse_immtrim (ds->opstr);
			} else if (ds->use_esil) {
				if (!hasanal) {
					r_anal_op (core->anal, &ds->analop,
						ds->at, core->block + i,
						core->blocksize - i, R_ANAL_OP_MASK_ESIL | R_ANAL_OP_MASK_HINT);
					hasanal = true;
				}
				if (*R_STRBUF_SAFEGET (&ds->analop.esil)) {
					free (ds->opstr);
					ds->opstr = strdup (R_STRBUF_SAFEGET (&ds->analop.esil));
				}
			} else if (ds->filter) {
				char *asm_str;
				RSpace *ofs = core->parser->flagspace;
				RSpace *fs = ds->flagspace_ports;
				if (ds->analop.type == R_ANAL_OP_TYPE_IO) {
					core->parser->notin_flagspace = NULL;
					core->parser->flagspace = fs;
				} else {
					if (fs) {
						core->parser->notin_flagspace = fs;
						core->parser->flagspace = fs;
					} else {
						core->parser->notin_flagspace = NULL;
						core->parser->flagspace = NULL;
					}
				}
				core->parser->hint = ds->hint;
				ds->hint = NULL;
				r_parse_filter (core->parser, ds->vat, core->flags, r_asm_op_get_asm (&ds->asmop),
					ds->str, sizeof (ds->str), core->print->big_endian);
				ds->opstr = strdup (ds->str);
				asm_str = colorize_asm_string (core, ds, true);
				core->parser->flagspace = ofs;
				free (ds->opstr);
				ds->opstr = asm_str;
				core->parser->flagspace = ofs; // ???
			} else {
				ds->opstr = strdup (r_asm_op_get_asm (&ds->asmop));
			}
			if (ds->immtrim) {
				free (ds->opstr);
				ds->opstr = strdup (r_asm_op_get_asm (&ds->asmop));
				r_parse_immtrim (ds->opstr);
			}
		}
		if (ds->asm_instr) {
			const char *opcolor = NULL;
			if (ds->show_color) {
				opcolor = r_print_color_op_type (core->print, ds->analop.type);
				r_cons_printf ("%s%s" Color_RESET "\n", opcolor, ds->opstr);
			} else {
				r_cons_println (ds->opstr);
			}
			R_FREE (ds->opstr);
		}
		if (ds->hint) {
			r_anal_hint_free (ds->hint);
			ds->hint = NULL;
		}
	}
	r_cons_break_pop ();
	ds_free (ds);
 err_offset:
	core->offset = old_offset;
	r_reg_arena_pop (core->anal->reg);

	return len;
}

R_API int r_core_print_disasm_json(RCore *core, ut64 addr, ut8 *buf, int nb_bytes, int nb_opcodes, PJ *pj) {
	RAsmOp asmop;
	RDisasmState *ds;
	RAnalFunction *f;
	int i, j, k, ret, line;
	ut64 old_offset = core->offset;
	ut64 at;
	int dis_opcodes = 0;
	int limit_by = 'b';
	char str[512];

	if (nb_opcodes != 0) {
		limit_by = 'o';
	}
	if (nb_opcodes) { // Disassemble `nb_opcodes` opcodes.
		if (nb_opcodes < 0) {
			int count, nbytes = 0;

			/* Backward disassembly of `nb_opcodes` opcodes:
			 * - We compute the new starting offset
			 * - Read at the new offset */
			nb_opcodes = -nb_opcodes;

			if (nb_opcodes > 0xffff) {
				eprintf ("Too many backward instructions\n");
				return false;
			}

			if (r_core_prevop_addr (core, core->offset, nb_opcodes, &addr)) {
				nbytes = old_offset - addr;
			} else if (!r_core_asm_bwdis_len (core, &nbytes, &addr, nb_opcodes)) {
				/* workaround to avoid empty arrays */
#define BWRETRY 0
#if BWRETRY
				nb_opcodes ++;
				if (!r_core_asm_bwdis_len (core, &nbytes, &addr, nb_opcodes)) {
#endif
					pj_end (pj);
					return false;
#if BWRETRY
				}
#endif
				nb_opcodes --;
			}
			count = R_MIN (nb_bytes, nbytes);
			if (count > 0) {
				r_io_read_at (core->io, addr, buf, count);
				r_io_read_at (core->io, addr+count, buf+count, nb_bytes-count);
			} else {
				if (nb_bytes > 0) {
					memset (buf, 0xff, nb_bytes);
				}
			}
		} else {
			// If we are disassembling a positive number of lines, enable dis_opcodes
			// to be used to finish the loop
			// If we are disasembling a negative number of lines, we just calculate
			// the equivalent addr and nb_size and scan a positive number of BYTES
			// so keep dis_opcodes = 0;
			dis_opcodes = 1;
			r_io_read_at (core->io, addr, buf, nb_bytes);
		}
	} else { // Disassemble `nb_bytes` bytes
		if (nb_bytes < 0) {
			//Backward disassembly of `nb_bytes` bytes
			nb_bytes = -nb_bytes;
			addr -= nb_bytes;
			r_io_read_at (core->io, addr, buf, nb_bytes);
		}
	}
	core->offset = addr;

	// XXX - is there a better way to reset a the analysis counter so that
	// when code is disassembled, it can actually find the correct offsets
	if (core->anal && core->anal->cur && core->anal->cur->reset_counter) {
		core->anal->cur->reset_counter (core->anal, addr);
	}
	// TODO: add support for anal hints
	// If using #bytes i = j
	// If using #opcodes, j is the offset from start address. i is the
	// offset in current disassembly buffer (256 by default)
	i = k = j = line = 0;
	// i = number of bytes
	// j = number of instructions
	// k = delta from addr
	ds = ds_init (core);
	bool result = false;

	for (;;) {
		bool end_nbopcodes, end_nbbytes;
		int skip_bytes_flag = 0, skip_bytes_bb = 0;

		at = addr + k;
		ds->hint = r_core_hint_begin (core, ds->hint, ds->at);
		r_asm_set_pc (core->assembler, at);
		// 32 is the biggest opcode length in intel
		// Make sure we have room for it
		if (dis_opcodes == 1 && i >= nb_bytes - 32) {
			// Read another nb_bytes bytes into buf from current offset
			r_io_read_at (core->io, at, buf, nb_bytes);
			i = 0;
		}

		if (limit_by == 'o') {
			if (j >= nb_opcodes) {
				break;
			}
		} else if (i >= nb_bytes) {
			break;
		}
		memset (&asmop, 0, sizeof (RAsmOp));
		ret = r_asm_disassemble (core->assembler, &asmop, buf + i, nb_bytes - i);
		if (ret < 1) {
			pj_o (pj);
			pj_kn (pj, "offset", at);
			pj_ki (pj, "size", 1);
			pj_ks (pj, "type", "invalid");
			pj_end (pj);
			i++;
			k++;
			j++;
			result = true;
			continue;
		}

		char opstr[256];
		strcpy (opstr, r_asm_op_get_asm (&asmop));

		ds->has_description = false;
		r_anal_op_fini (&ds->analop);
		r_anal_op (core->anal, &ds->analop, at, buf + i, nb_bytes - i, R_ANAL_OP_MASK_ALL);

		if (ds->pseudo) {
			r_parse_parse (core->parser, opstr, opstr);
		}

		// f = r_anal_get_fcn_in (core->anal, at,
		f = fcnIn (ds, at, R_ANAL_FCN_TYPE_FCN | R_ANAL_FCN_TYPE_SYM | R_ANAL_FCN_TYPE_LOC);
		if (ds->varsub && f) {
			core->parser->varlist = r_anal_var_list_dynamic;
			int ba_len = r_strbuf_length (&asmop.buf_asm) + 128;
			char *ba = malloc (ba_len);
			if (ba) {
				strcpy (ba, r_asm_op_get_asm (&asmop));
				r_parse_varsub (core->parser, f, at, ds->analop.size,
						ba, ba, ba_len);
				r_asm_op_set_asm (&asmop, ba);
				free (ba);
			}
		}
		ds->oplen = r_asm_op_get_size (&asmop);
		ds->at = at;
		if (ds->midflags) {
			skip_bytes_flag = handleMidFlags (core, ds, false);
		}
		if (ds->midbb) {
			skip_bytes_bb = handleMidBB (core, ds);
		}
		if (skip_bytes_flag && ds->midflags > R_MIDFLAGS_SHOW) {
			ds->oplen = ret = skip_bytes_flag;
		}
		if (skip_bytes_bb && skip_bytes_bb < ret) {
			ds->oplen = ret = skip_bytes_bb;
		}
		{
			ut64 killme = UT64_MAX;
			bool be = core->print->big_endian;
			if (r_io_read_i (core->io, ds->analop.ptr, &killme, ds->analop.refptr, be)) {
				core->parser->relsub_addr = killme;
			}
		}
		{
			char *aop = r_asm_op_get_asm (&asmop);
			char *buf = malloc (strlen (aop) + 128);
			if (buf) {
				strcpy (buf, aop);
				r_parse_filter (core->parser, ds->vat, core->flags, buf,
					str, sizeof (str), core->print->big_endian);
				r_asm_op_set_asm (&asmop, buf);
				free (buf);
			}
		}

		pj_o (pj);
		pj_kn (pj, "offset", at);
		if (ds->analop.ptr != UT64_MAX) {
			pj_kn (pj, "ptr", ds->analop.ptr);
		}
		if (ds->analop.val != UT64_MAX) {
			pj_kn (pj, "val", ds->analop.val);
		}
		pj_k (pj, "esil"); // split key and value to allow empty strings
		pj_s (pj, R_STRBUF_SAFEGET (&ds->analop.esil));
		pj_kb (pj, "refptr", ds->analop.refptr);
		pj_kn (pj, "fcn_addr", f ? f->addr : 0);
		pj_kn (pj, "fcn_last", f ? f->addr + r_anal_fcn_size (f) - ds->oplen : 0);
		pj_ki (pj, "size", ds->analop.size);
		pj_ks (pj, "opcode", opstr);
		pj_ks (pj, "disasm", str);
		pj_ks (pj, "bytes", r_asm_op_get_hex (&asmop));
		pj_ks (pj, "family", r_anal_op_family_to_string (ds->analop.family));
		pj_ks (pj, "type", r_anal_optype_to_string (ds->analop.type));
		// indicate a relocated address
		RBinReloc *rel = getreloc (core, ds->at, ds->analop.size);
		// reloc is true if address in reloc table
		pj_kb (pj, "reloc", rel);
		// wanted the numerical values of the type information
		pj_kn (pj, "type_num", ds->analop.type);
		pj_kn (pj, "type2_num", ds->analop.type2);
		// handle switch statements
		if (ds->analop.switch_op && r_list_length (ds->analop.switch_op->cases) > 0) {
			// XXX - the java caseop will still be reported in the assembly,
			// this is an artifact to make ensure the disassembly is properly
			// represented during the analysis
			RListIter *iter;
			RAnalCaseOp *caseop;
			pj_k (pj, "switch");
			pj_a (pj);
			r_list_foreach (ds->analop.switch_op->cases, iter, caseop ) {
				pj_o (pj);
				pj_kn (pj, "addr", caseop->addr);
				pj_kN (pj, "value", (st64) caseop->value);
				pj_kn (pj, "jump", caseop->jump);
				pj_end (pj);
			}
			pj_end (pj);
		}
		if (ds->analop.jump != UT64_MAX ) {
			pj_kN (pj, "jump", ds->analop.jump);
			if (ds->analop.fail != UT64_MAX) {
				pj_kn (pj, "fail", ds->analop.fail);
			}
		}
		/* add flags */
		{
			const RList *flags = r_flag_get_list (core->flags, at);
			RFlagItem *flag;
			RListIter *iter;
			if (flags && !r_list_empty (flags)) {
				pj_k (pj, "flags");
				pj_a (pj);
				r_list_foreach (flags, iter, flag) {
					pj_s (pj, flag->name);
				}
				pj_end (pj);
			}
		}
		/* add comments */
		{
			// TODO: slow because we are decoding and encoding b64
			char *comment = r_meta_get_string (core->anal, R_META_TYPE_COMMENT, at);
			if (comment) {
				char *b64comment = sdb_encode ((const ut8*)comment, -1);
				pj_ks (pj, "comment", b64comment);
				free (comment);
				free (b64comment);
			}
		}
		/* add xrefs */
		{
			RAnalRef *ref;
			RListIter *iter;
			RList *xrefs = r_anal_xrefs_get (core->anal, at);
			if (xrefs && !r_list_empty (xrefs)) {
				pj_k (pj, "xrefs");
				pj_a (pj);
				r_list_foreach (xrefs, iter, ref) {
					pj_o (pj);
					pj_kn (pj, "addr", ref->addr);
					pj_ks (pj, "type", r_anal_xrefs_type_tostring (ref->type));
					pj_end (pj);
				}
				pj_end (pj);
			}
			r_list_free (xrefs);
		}

		pj_end (pj);
		i += ds->oplen + asmop.payload + (ds->asmop.payload % ds->core->assembler->dataalign); // bytes
		k += ds->oplen + asmop.payload + (ds->asmop.payload % ds->core->assembler->dataalign); // delta from addr
		j++; // instructions
		line++;

		end_nbopcodes = dis_opcodes == 1 && nb_opcodes > 0 && line>=nb_opcodes;
		end_nbbytes = dis_opcodes == 0 && nb_bytes > 0 && i>=nb_bytes;
		result = true;
		if (end_nbopcodes || end_nbbytes) {
			break;
		}
	}
	// r_cons_printf ("]");
	core->offset = old_offset;
	r_anal_op_fini (&ds->analop);
	ds_free (ds);
	if (!result) {
		pj_o (pj);
		pj_end (pj);
		result = true;
	}
	return result;
}

R_API int r_core_print_disasm_all(RCore *core, ut64 addr, int l, int len, int mode) {
	const bool scr_color = r_config_get_i (core->config, "scr.color");
	int i, ret, count = 0;
	ut8 *buf = core->block;
	char str[128];
	RAsmOp asmop;
	if (l < 1) {
		l = len;
	}
	RDisasmState *ds = ds_init (core);
	if (l > core->blocksize || addr != core->offset) {
		buf = malloc (l + 1);
		r_io_read_at (core->io, addr, buf, l);
	}
	if (mode == 'j') {
		r_cons_print ("[");
	}
	r_cons_break_push (NULL, NULL);
	for (i = 0; i < l; i++) {
		ds->at = addr + i;
		ds->vat = r_core_pava (core, ds->at);
		r_asm_set_pc (core->assembler, ds->vat);
		if (r_cons_is_breaked ()) {
			break;
		}
		ret = r_asm_disassemble (core->assembler, &asmop, buf + i, l - i);
		if (ret < 1) {
			switch (mode) {
			case 'j':
			case '=':
				break;
			case 'i':
				r_cons_printf ("???\n");
				break;
			default:
				r_cons_printf ("0x%08"PFMT64x" ???\n", ds->vat);
				break;
			}
		} else {
			count ++;
			switch (mode) {
			case 'i':
				r_parse_filter (core->parser, ds->vat, core->flags, r_asm_op_get_asm (&asmop),
						str, sizeof (str), core->print->big_endian);
				if (scr_color) {
					RAnalOp aop;
					RAnalFunction *f = fcnIn (ds, ds->vat, R_ANAL_FCN_TYPE_NULL);
					r_anal_op (core->anal, &aop, addr, buf+i, l-i, R_ANAL_OP_MASK_ALL);
					char *buf_asm = r_print_colorize_opcode (core->print, str,
							core->cons->context->pal.reg, core->cons->context->pal.num, false, f ? f->addr : 0);
					if (buf_asm) {
						r_cons_printf ("%s%s\n", r_print_color_op_type (core->print, aop.type), buf_asm);
						free (buf_asm);
					}
				} else {
					r_cons_println (r_asm_op_get_asm (&asmop));
				}
				break;
			case '=':
				if (i < 28) {
					char *str = r_str_newf ("0x%08"PFMT64x" %60s  %s\n", ds->vat, "", r_asm_op_get_asm (&asmop));
					char *sp = strchr (str, ' ');
					if (sp) {
						char *end = sp + 60 + 1;
						const char *src = r_asm_op_get_hex (&asmop);
						char *dst = sp + 1 + (i * 2);
						int len = strlen (src);
						if (dst < end) {
							if (dst + len >= end) {
								len = end - dst;
								dst[len] = '.';
							}
							memcpy (dst, src, len);
						}
					}
					r_cons_strcat (str);
					free (str);
				}
				break;
			case 'j':
				r_cons_printf ("{\"addr\":%08"PFMT64d",\"bytes\":\"%s\",\"inst\":\"%s\"}%s",
					addr + i, r_asm_op_get_hex (&asmop), r_asm_op_get_asm (&asmop), ",");
				break;
			default:
				r_cons_printf ("0x%08"PFMT64x" %20s  %s\n",
						addr + i, r_asm_op_get_hex (&asmop),
						r_asm_op_get_asm (&asmop));
			}
		}
	}
	r_cons_break_pop ();
	if (buf != core->block) {
		free (buf);
	}
	if (mode == 'j') {
		r_cons_printf ("{}]\n");
	}
	ds_free (ds);
	return count;
}

static inline bool pdi_check_end(int nb_opcodes, int nb_bytes, int i, int j) {
	if (nb_opcodes > 0) {
		if (nb_bytes > 0) {
			return j < nb_opcodes && i < nb_bytes;
		}
		return j < nb_opcodes;
	}
	return i < nb_bytes;
}

R_API int r_core_disasm_pdi(RCore *core, int nb_opcodes, int nb_bytes, int fmt) {
	int show_offset = r_config_get_i (core->config, "asm.offset");
	int show_bytes = r_config_get_i (core->config, "asm.bytes");
	int decode = r_config_get_i (core->config, "asm.decode");
	int filter = r_config_get_i (core->config, "asm.filter");
	int show_color = r_config_get_i (core->config, "scr.color");
	bool asm_ucase = r_config_get_i (core->config, "asm.ucase");
	int esil = r_config_get_i (core->config, "asm.esil");
	int flags = r_config_get_i (core->config, "asm.flags");
	bool asm_immtrim = r_config_get_i (core->config, "asm.imm.trim");
	int i = 0, j, ret, err = 0;
	ut64 old_offset = core->offset;
	RAsmOp asmop;
	const char *color_reg = R_CONS_COLOR_DEF (reg, Color_YELLOW);
	const char *color_num = R_CONS_COLOR_DEF (num, Color_CYAN);
	const int addrbytes = core->io->addrbytes;

	if (fmt == 'e') {
		show_bytes = 0;
		decode = 1;
	}
	if (!nb_opcodes && !nb_bytes) {
		return 0;
	}
	if (!nb_opcodes) {
		nb_opcodes = -1;
		if (nb_bytes < 0) {
			// Backward disasm `nb_bytes` bytes
			nb_bytes = -nb_bytes;
			core->offset -= nb_bytes;
			r_io_read_at (core->io, core->offset, core->block, nb_bytes);
		}
	} else if (!nb_bytes) {
		if (nb_opcodes < 0) {
			ut64 start;
			/* Backward disassembly of `ilen` opcodes
			* - We compute the new starting offset
			* - Read at the new offset */
			nb_opcodes = -nb_opcodes;
			if (r_core_prevop_addr (core, core->offset, nb_opcodes, &start)) {
				// We have some anal_info.
				nb_bytes = core->offset - start;
			} else {
				// anal ignorance.
				r_core_asm_bwdis_len (core, &nb_bytes, &core->offset,
					nb_opcodes);
			}
			nb_bytes *= core->io->addrbytes;
			if (nb_bytes > core->blocksize) {
				r_core_block_size (core, nb_bytes);
			}
			r_io_read_at (core->io, core->offset, core->block, nb_bytes);
		} else {
			// workaround for the `for` loop below
			nb_bytes = core->blocksize;
		}
	}

	// XXX - is there a better way to reset a the analysis counter so that
	// when code is disassembled, it can actually find the correct offsets
	if (core->anal && core->anal->cur && core->anal->cur->reset_counter) {
		core->anal->cur->reset_counter (core->anal, core->offset);
	}

	int len = (nb_opcodes + nb_bytes) * 5;
	if (core->fixedblock) {
		len = core->blocksize;
	}
	if (len > core->blocksize) {
		r_core_block_size (core, len);
		r_core_block_read (core);
	}
	r_cons_break_push (NULL, NULL);

	int midflags = r_config_get_i (core->config, "asm.flags.middle");
	int midbb = r_config_get_i (core->config, "asm.bb.middle");
	i = 0;
	j = 0;
toro:
	for (; pdi_check_end (nb_opcodes, nb_bytes, addrbytes * i, j); j++) {
		RFlagItem *item;
		if (r_cons_is_breaked ()) {
			err = 1;
			break;
		}
		RAnalMetaItem *meta = r_meta_find (core->anal, core->offset + i,
			R_META_TYPE_ANY, R_META_WHERE_HERE);
		if (meta && meta->size > 0) {
			switch (meta->type) {
			case R_META_TYPE_DATA:
				r_cons_printf (".data: %s\n", meta->str);
				i += meta->size;
				continue;
			case R_META_TYPE_STRING:
				r_cons_printf (".string: %s\n", meta->str);
				i += meta->size;
				continue;
			case R_META_TYPE_FORMAT:
				r_cons_printf (".format : %s\n", meta->str);
				i += meta->size;
				continue;
			case R_META_TYPE_MAGIC:
				r_cons_printf (".magic : %s\n", meta->str);
				i += meta->size;
				continue;
			case R_META_TYPE_RUN:
				/* TODO */
				break;
			}
		}
		r_asm_set_pc (core->assembler, core->offset + i);
		ret = r_asm_disassemble (core->assembler, &asmop, core->block + addrbytes * i,
			core->blocksize - addrbytes * i);
		if (midflags || midbb) {
			RDisasmState ds = {
				.oplen = ret,
				.at = core->offset + i,
				.midflags = midflags
			};
			int skip_bytes_flag = 0, skip_bytes_bb = 0;
			if (midflags) {
				skip_bytes_flag = handleMidFlags (core, &ds, true);
			}
			if (midbb) {
				skip_bytes_bb = handleMidBB (core, &ds);
			}
			if (skip_bytes_flag && midflags > R_MIDFLAGS_SHOW) {
				asmop.size = ret = skip_bytes_flag;
			}
			if (skip_bytes_bb && skip_bytes_bb < ret) {
				asmop.size = ret = skip_bytes_bb;
			}
		}
		if (fmt == 'C') {
			char *comment = r_meta_get_string (core->anal, R_META_TYPE_COMMENT, core->offset + i);
			if (comment) {
				r_cons_printf ("0x%08"PFMT64x " %s\n", core->offset + i, comment);
				free (comment);
			}
			i += ret;
			continue;
		}
		if (flags) {
			if (fmt != 'e') { // pie
				item = r_flag_get_i (core->flags, core->offset + i);
				if (item) {
					if (show_offset) {
						r_cons_printf ("0x%08"PFMT64x "  ", core->offset + i);
					}
					r_cons_printf ("  %s:\n", item->name);
				}
			} // do not show flags in pie
		}
		ut64 at = core->offset + i;
		if (show_offset) {
			const int show_offseg = (core->print->flags & R_PRINT_FLAGS_SEGOFF) != 0;
			const int show_offdec = (core->print->flags & R_PRINT_FLAGS_ADDRDEC) != 0;
			unsigned int seggrn = r_config_get_i (core->config, "asm.seggrn");

			r_print_offset_sg (core->print, at, 0, show_offseg, seggrn, show_offdec, 0, NULL);
		}
		// r_cons_printf ("0x%08"PFMT64x"  ", core->offset+i);
		if (ret < 1) {
			err = 1;
			ret = asmop.size;
			if (ret < 1) {
				ret = 1;
			}
			if (show_bytes) {
				r_cons_printf ("%18s%02x  ", "", core->block[i]);
			}
			r_cons_println ("invalid"); // ???");
		} else {
			if (show_bytes) {
				r_cons_printf ("%20s  ", r_asm_op_get_hex (&asmop));
			}
			ret = asmop.size;
			if (!asm_immtrim && (decode || esil)) {
				RAnalOp analop = {
					0
				};
				char *tmpopstr, *opstr = NULL;
				r_anal_op (core->anal, &analop, core->offset + i,
					core->block + addrbytes * i, core->blocksize - addrbytes * i, R_ANAL_OP_MASK_ALL);
				tmpopstr = r_anal_op_to_string (core->anal, &analop);
				if (fmt == 'e') { // pie
					char *esil = (R_STRBUF_SAFEGET (&analop.esil));
					r_cons_println (esil);
				} else {
					if (decode) {
						opstr = tmpopstr? tmpopstr: r_asm_op_get_asm (&(asmop));
					} else if (esil) {
						opstr = (R_STRBUF_SAFEGET (&analop.esil));
					}
					if (asm_immtrim ) {
						r_parse_immtrim (opstr);
					}
					r_cons_println (opstr);
				}
			} else {
				char opstr[128] = {
					0
				};
				char *asm_str = r_asm_op_get_asm (&asmop);
				if (asm_ucase) {
					r_str_case (asm_str, 1);
				}
				if (asm_immtrim) {
					r_parse_immtrim (asm_str);
				}
				if (filter) {
					core->parser->hint = r_anal_hint_get (core->anal, at);
					r_parse_filter (core->parser, at, core->flags,
						asm_str, opstr, sizeof (opstr) - 1, core->print->big_endian);
					asm_str = (char *)&opstr;
				}
				if (show_color) {
					RAnalOp aop = {
						0
					};
					RAnalFunction *f = r_anal_get_fcn_in (core->anal, core->offset + i, R_ANAL_FCN_TYPE_NULL);
					r_anal_op (core->anal, &aop, core->offset + i,
						core->block + addrbytes * i, core->blocksize - addrbytes * i, R_ANAL_OP_MASK_BASIC);
					asm_str = r_print_colorize_opcode (core->print, asm_str, color_reg, color_num, false, f ? f->addr : 0);
					r_cons_printf ("%s%s"Color_RESET "\n",
						r_print_color_op_type (core->print, aop.type),
						asm_str);
					free (asm_str);
					r_anal_op_fini (&aop);
				} else {
					r_cons_println (asm_str);
				}
			}
		}
		i += ret;
	}
	if (nb_opcodes > 0 && j < nb_opcodes) {
		r_core_seek (core, core->offset + i, 1);
		i = 0;
		goto toro;
	}
	r_cons_break_pop ();
	r_core_seek (core, old_offset, 1);
	return err;
}
