"""Focused tests for source lowering; assembler parity is checked separately."""

import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "prepare-wasm-original.py"
SPEC = importlib.util.spec_from_file_location("prepare_wasm_original", SCRIPT)
ADAPTER = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = ADAPTER
SPEC.loader.exec_module(ADAPTER)


class PrepareWasmTests(unittest.TestCase):
    def lower(self, source, definitions=""):
        sources = {"structs.inc": definitions.encode("latin1")}
        layouts = ADAPTER.collect_layouts(sources)
        return ADAPTER.lower_source("probe.asm", source.encode("latin1"), layouts)[0].decode("latin1")

    def test_typed_stack_offsets_and_explicit_override(self):
        result = self.lower("fn proc far\np = dword ptr -4\nc = byte ptr -5\n"
                            "les bx,[bp+p]\nmov ax,word ptr [bp+p+2]\n"
                            "cmp [bp+c],1\nfn endp\n")
        self.assertIn("p = -4", result)
        self.assertIn("les bx,dword ptr [bp+p]", result)
        self.assertIn("mov ax,word ptr [bp+p+2]", result)
        self.assertIn("cmp byte ptr [bp+c],1", result)
        self.assertNotIn("word ptr dword ptr", result)

    def test_structure_members_retain_scalar_width(self):
        definitions = "V struc\nx dw ?\ny dw ?\nV ends\nS struc\na db ?\nv V <>\nS ends\n"
        result = self.lower("fn proc far\np = S ptr -30\n"
                            "mov ax,[bp+p.v.y]\nmov [bx+S.v.y],0\n"
                            "add ax,S.v.y\nfn endp\n", definitions)
        self.assertIn("mov ax,word ptr [bp+p+3]", result)
        self.assertIn("mov word ptr [bx+3],0", result)
        self.assertIn("db 5\r\ndw S.v.y", result)

    def test_union_uses_maximum_member_size_and_overlay_offsets(self):
        definitions = ("Words struc\na dw ?\nb dw ?\nWords ends\n"
                       "Bytes struc\nx db ?\ny db ?\nBytes ends\n"
                       "Overlay union\nw Words <>\nb Bytes <>\nOverlay ends\n")
        layouts = ADAPTER.collect_layouts({"structs.inc": definitions.encode()})
        self.assertEqual(layouts["overlay"].size, 4)
        generated, _ = ADAPTER.lower_source("structs.inc", definitions.encode(), layouts)
        self.assertIn(b"Overlay struc\r\n__wasm_storage_Overlay db 4 dup (?)", generated)
        result = self.lower("f proc far\nu = Overlay ptr -8\n"
                            "mov al,[bp+u.b.y]\nmov ax,[bp+u.w.b]\nf endp", definitions)
        self.assertIn("byte ptr [bp+u+1]", result)
        self.assertIn("word ptr [bp+u+2]", result)

    def test_array_layout_and_size_immediate(self):
        definitions = "Sample struc\nx db 3 dup (?)\ny dw 2 dup (?)\nSample ends\n"
        result = self.lower("add ax,size Sample\nmov ax,[bx+Sample.y]\n", definitions)
        self.assertIn("db 5\r\ndw size Sample", result)
        self.assertIn("mov ax,word ptr [bx+3]", result)
        self.assertEqual(ADAPTER.collect_layouts({"s.inc": definitions.encode()})["sample"].size, 7)

    def test_alias_scope_is_reset_at_each_procedure(self):
        result = self.lower("a proc far\nv = byte ptr -2\ncmp [bp+v],1\na endp\n"
                            "b proc far\nv = word ptr -2\ncmp [bp+v],1\nb endp\n")
        self.assertIn("cmp byte ptr [bp+v],1", result)
        self.assertIn("cmp word ptr [bp+v],1", result)

    def test_preserves_repne_string_prefix_byte(self):
        result = self.lower("repne movsw\nrepnz stosb\nrepe lodsb\nrepne scasb\nrep movsw\n")
        self.assertIn("db 242\r\nmovsw", result)
        self.assertIn("db 242\r\nstosb", result)
        self.assertIn("db 243\r\nlodsb", result)
        self.assertIn("repne scasb", result)
        self.assertIn("rep movsw", result)

    def test_accumulator_opcodes_match_tasm_encoding_probe(self):
        # Measured with TASM32 /m2 /s /zn: accumulator opcodes are used
        # in both SMART states even when the 83h encoding has equal length.
        source = "cmp ax,1\nadd ax,-1\nand ax,1\ntest ax,1\n"
        result = self.lower(source)
        self.assertEqual(result, "db 61\r\ndw 1\r\ndb 5\r\ndw -1\r\n"
                                 "db 37\r\ndw 1\r\ndb 169\r\ndw 1\r\n")

    def test_smart_controls_logical_word_immediate(self):
        result = self.lower("nosmart\nand bx,1\nsmart\nand bx,1\nnosmart\nxor dx,2\n")
        # TASM NOSMART emits 81 E3 01 00; SMART permits 83 E3 01.
        self.assertIn("db 129, 227\r\ndw 1", result)
        self.assertIn("and bx,1", result)
        self.assertIn("db 129, 242\r\ndw 2", result)
        self.assertNotIn("\r\nnosmart\r\n", result)

    def test_memory_operand_is_not_mistaken_for_immediate(self):
        result = self.lower("or ax,flags\nand ax,word ptr [bp-2]\nadd ax,bx\n")
        self.assertEqual(result, "or ax,flags\r\nand ax,word ptr [bp-2]\r\nadd ax,bx\r\n")

    def test_symbolic_immediate_keeps_assembler_fixup(self):
        result = self.lower("add ax,offset table\ncmp ax,seg target\n")
        self.assertIn("db 5\r\ndw offset table", result)
        self.assertIn("db 61\r\ndw seg target", result)

    def test_sbb_and_xchg_preserve_tasm_register_direction(self):
        result = self.lower("sbb dx,bx\nsbb al,bl\nxchg di,si\nxchg ax,cx\n")
        self.assertEqual(result, "db 27, 211\r\ndb 26, 195\r\ndb 135, 254\r\nxchg ax,cx\r\n")

    def test_local_far_transfers_are_not_shortened(self):
        result = self.lower("caller proc far\ncall callee\njmp callee\n"
                            "call near ptr callee\ncall external\n"
                            "call far ptr external\ncaller endp\ncallee proc far\nretf\ncallee endp\n")
        self.assertIn("db 154\r\ndw offset callee, seg callee", result)
        self.assertIn("db 234\r\ndw offset callee, seg callee", result)
        self.assertIn("call near ptr callee", result)
        self.assertIn("call external", result)
        self.assertIn("db 154\r\ndw offset external, seg external", result)

    def test_explicit_short_jump_is_preserved(self):
        result = self.lower("nosmart\njmp target\njmp short target\n"
                            "smart\njmp target\ntarget:\n")
        self.assertIn("jmp near ptr target", result)
        self.assertIn("jmp short target", result)
        self.assertIn("; WASM adapter: smart\r\njmp target", result)

    def test_comment_bytes_and_quoted_semicolons_are_preserved(self):
        source = "db 'a;b' ; literal\r\nmov ax,85h ; '\x85'\r\n"
        result = self.lower(source)
        self.assertEqual(result, source)
        self.assertEqual(len(result.split("\r\n")), 3)

    def test_unknown_structure_member_is_rejected(self):
        definitions = "S struc\na dw ?\nS ends\n"
        with self.assertRaisesRegex(ADAPTER.AdapterError, "unknown stack member"):
            self.lower("f proc far\nx = S ptr -2\nmov ax,[bp+x.missing]", definitions)

    def test_unknown_structure_layout_is_rejected(self):
        with self.assertRaisesRegex(ADAPTER.AdapterError, "unknown structure member type"):
            ADAPTER.collect_layouts({"a.inc": b"S struc\nx Missing <>\nS ends\n"})
        with self.assertRaisesRegex(ADAPTER.AdapterError, "unsupported structure initializer"):
            ADAPTER.collect_layouts({"a.inc": b"S struc\nx db 1,2\nS ends\n"})

    def test_output_is_deterministic_and_sources_are_immutable(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "original"
            source.mkdir()
            original = b".model medium\r\nnosmart\r\nf proc far\r\nx = word ptr -2\r\ncmp [bp+x],1\r\nretf\r\nf endp\r\nend\r\n"
            (source / "game.asm").write_bytes(original)
            first = ADAPTER.prepare(source, root / "first")
            second = ADAPTER.prepare(source, root / "second")
            self.assertEqual(first, second)
            self.assertEqual((source / "game.asm").read_bytes(), original)
            self.assertEqual((root / "first/game.asm").read_bytes(), (root / "second/game.asm").read_bytes())
            self.assertEqual(json.loads((root / "first/manifest.json").read_text()), first)
            (root / "first/game.asm").unlink()
            ADAPTER.prepare(source, root / "first")
            self.assertTrue((root / "first/game.asm").exists())
            self.assertEqual((source / "game.asm").read_bytes(), original)
            with self.assertRaisesRegex(ADAPTER.AdapterError, "must differ"):
                ADAPTER.prepare(source, source)


if __name__ == "__main__":
    unittest.main()
