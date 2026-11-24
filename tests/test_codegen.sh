#!/usr/bin/env bash
set -u

source "$(dirname "$0")/environment.sh"

failures=0
for source in "$root"/tests/shapes/*.rz; do
    name="$(basename "$source" .rz)"
    asm="$work/$name.asm"
    if ! fits_host "$source"; then
        echo "skip $name"
        continue
    fi
    if ! "$rzc" "$source" -S -o "$asm" 2> "$work/$name.log"; then
        echo "FAIL $name: rzc failed: $(cat "$work/$name.log")"
        failures=$((failures + 1))
        continue
    fi
    problems=""
    while IFS= read -r line; do
        case "$line" in
            "// asm: "*)
                grep -qF -- "${line#// asm: }" "$asm" || problems+=" missing [${line#// asm: }]"
                ;;
            "// platform: "*)
                ;;
            "// absent: "*)
                grep -qF -- "${line#// absent: }" "$asm" && problems+=" unexpected [${line#// absent: }]"
                ;;
            *)
                break
                ;;
        esac
    done < "$source"
    if [ -z "$problems" ]; then
        echo "ok   $name"
    else
        echo "FAIL $name:$problems"
        failures=$((failures + 1))
    fi
done

source="$root/tests/programs/optimizer_semantics.rz"
zen="$work/optimizer_zen3.asm"
portable="$work/optimizer_portable.asm"
if "$rzc" "$source" -march=znver3 -S -o "$zen" &&
   "$rzc" "$source" -O -S -o "$portable" &&
   grep -q '^    sarx ' "$zen" && grep -q '^    shrx ' "$zen" && grep -q '^    shlx ' "$zen" &&
   grep -q '^    vsubsd ' "$zen" &&
   ! grep -qE '^    (sarx|shrx|shlx|vsubsd) ' "$portable"; then
    echo "ok   Zen 3 instructions require the explicit target"
else
    echo "FAIL Zen 3 instruction selection"
    failures=$((failures + 1))
fi
source="$root/tests/programs/loop_optimization.rz"
if "$rzc" "$source" -O -S -o "$portable" &&
   "$rzc" "$source" -march=znver3 -S -o "$zen" &&
   grep -q '^    pmuludq ' "$portable" && grep -q '^    movdqu ' "$portable" &&
   grep -q '^    vpmuludq ' "$zen" && grep -q '^    vmovdqu ' "$zen" && grep -qF 'vmovdqu [' "$zen" &&
   grep -q '^    vzeroupper' "$zen" && ! grep -q 'ymm' "$portable" &&
   [ "$(awk '/^\$product_loop:/{body=1;next} /^\$/{body=0} body && /mulsd /{count++} END{print count+0}' "$zen")" -eq 3 ]; then
    echo "ok   vector reductions and loop product reuse"
else
    echo "FAIL vector reductions or loop product reuse"
    failures=$((failures + 1))
fi
source="$root/tests/programs/optimization_motion.rz"
plain="$work/optimization_motion_plain.asm"
if "$rzc" "$source" -S -o "$plain" &&
   "$rzc" "$source" -O -S -o "$portable" &&
   grep -qF 'call $min' "$plain" && ! grep -qF 'call $min' "$portable" &&
   grep -qF 'call $choose_argument' "$portable" &&
   awk '/^\$helper_loop:/{body=1;next} /^\$/{body=0} body && /cmovl /{selected=1} END{exit !selected}' "$portable" &&
   awk '/^\$invariant_divide:/{body=1;next} /^\$/{body=0} body && /idiv / && !top{divided=1} body && /^\.@for[0-9]+:/{top=1} END{exit !(top && divided)}' "$portable"; then
    echo "ok   small direct calls and guarded loop invariants"
else
    echo "FAIL small direct calls or guarded loop invariants"
    failures=$((failures + 1))
fi
source="$root/tests/programs/packed_double_loops.rz"
if "$rzc" "$source" -S -o "$plain" &&
   "$rzc" "$source" -O -S -o "$portable" &&
   "$rzc" "$source" -march=znver3 -S -o "$zen" &&
   grep -q '^    vcmppd ymm' "$zen" && grep -q '^    vandpd ymm' "$zen" &&
   grep -q '^    cmpunordpd xmm' "$portable" && grep -q '^    mulpd xmm' "$portable" &&
   ! grep -qE '^    (vcmppd|cmpunordpd) ' "$plain" &&
   ! grep -q '^    vcmppd ' "$portable" &&
   awk '/^\$dependent_total:/{body=1;next} /^\$/{body=0} body && /doublegroup/{packed=1} END{exit packed}' "$zen"; then
    echo "ok   masked double loops and scalar dependency fallback"
else
    echo "FAIL masked double loops or scalar dependency fallback"
    failures=$((failures + 1))
fi
source="$root/tests/programs/register_pressure.rz"
if "$rzc" "$source" -S -o "$plain" &&
   "$rzc" "$source" -O -S -o "$portable" &&
   grep -qF '[rsp + .left]' "$plain" &&
   awk '/^\$hot_registers:/{body=1;next} /^\$/{body=0} body && /\[rsp \+ \.(left|right|i)\]/{spill=1} body && /call \$perturb/{helper=1} END{exit !(helper && !spill)}' "$portable" &&
   awk '/^\$tail_with_pressure:/{body=1;next} /^\$/{body=0} body && /jmp \.@recur/{tail=1} END{exit !tail}' "$portable"; then
    echo "ok   loop-heavy register priority and tail calls"
else
    echo "FAIL loop-heavy register priority"
    failures=$((failures + 1))
fi

exit $failures
