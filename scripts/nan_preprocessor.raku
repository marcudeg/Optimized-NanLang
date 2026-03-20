#!/usr/bin/env raku
use v6;

grammar NanLineGrammar {
    token TOP { ^ <line> $ }
    token line {
        <blank>
      | <comment>
      | <magic_directive>
      | <macro_call>
      | <pulse_stmt>
      | <emit_stmt>
      | <jitter_stmt>
      | <halt_stmt>
      | <macro_invalid>
      | <pulse_invalid>
      | <emit_invalid>
      | <other>
    }

    token blank { \h* }
    token comment { \h* [ '//' <-[\n]>* | '/*' <-[\n]>* '*/' ] \h* }
    token ident { <[A..Za..z_]> <[A..Za..z0..9_]>* }
    token trailing_comment { \h* [ '//' <-[\n]>* ]? \h* }

    token magic_directive {
        \h* [ 'NAN3' | '@nan_magic' \h* '(' \h* 'NAN3' \h* ')' ] \h* ';'?
        <trailing_comment>
    }

    token macro_call {
        \h* '@macro' \h* '(' \h* <name=.ident> \h* ')' \h* ';'?
        <trailing_comment>
    }

    token signal_literal { '_>>_' | '_>_' | '^' | '_' | '?' }
    token signal_expr { <signal_literal> | <ident> }

    token pulse_stmt {
        \h* 'pulse' \h+ <signal=.signal_expr> \h* ';'
        <trailing_comment>
    }

    token emit_stmt {
        \h* 'emit' \h+ <signal=.signal_expr> \h* ';'
        <trailing_comment>
    }

    token jitter_stmt { \h* 'jitter' \h* ';' <trailing_comment> }
    token halt_stmt   { \h* 'halt'   \h* ';' <trailing_comment> }

    token macro_invalid { \h* '@macro' <-[\n]>* }
    token pulse_invalid { \h* 'pulse' <|w> <-[\n]>* }
    token emit_invalid  { \h* 'emit'  <|w> <-[\n]>* }

    token other { .* }
}

class NanLineActions {
    method TOP($/)             { make $<line>.made }
    method line($/) {
        for <
            blank comment magic_directive macro_call pulse_stmt emit_stmt
            jitter_stmt halt_stmt macro_invalid pulse_invalid emit_invalid other
        > -> $name {
            if $/{$name}:exists && $/{$name}.defined {
                make $/{$name}.made;
                return;
            }
        }
        make { type => 'invalid' };
    }
    method blank($/)           { make { type => 'blank' } }
    method comment($/)         { make { type => 'comment' } }
    method magic_directive($/) { make { type => 'magic' } }
    method macro_call($/)      { make { type => 'macro', name => ~$<name> } }
    method pulse_stmt($/)      { make { type => 'pulse', signal => ~$<signal> } }
    method emit_stmt($/)       { make { type => 'emit',  signal => ~$<signal> } }
    method jitter_stmt($/)     { make { type => 'jitter' } }
    method halt_stmt($/)       { make { type => 'halt' } }
    method macro_invalid($/)   { make { type => 'macro-invalid' } }
    method pulse_invalid($/)   { make { type => 'pulse-invalid' } }
    method emit_invalid($/)    { make { type => 'emit-invalid' } }
    method other($/)           { make { type => 'other' } }
}

my constant %MACRO_EXPANSIONS = (
    'PULSE_HIGH' => [
        '@0xF0 = 0x03; // OP_LOAD',
        '@0xF1 = 0x01; // High signal',
        '@0xF2 = 0x04; // OP_STORE',
        '@0xF3 = 0x10; // Pulse lane',
        'pulse ^;',
        'emit ^;',
    ],
    'PULSE_LOW' => [
        '@0xF0 = 0x03; // OP_LOAD',
        '@0xF1 = 0x00; // Low signal',
        '@0xF2 = 0x04; // OP_STORE',
        '@0xF3 = 0x10; // Pulse lane',
        'pulse _;',
        'emit _;',
    ],
    'PULSE_FALL' => [
        '@0xF0 = 0x03; // OP_LOAD',
        '@0xF1 = 0x10; // Falling edge marker',
        '@0xF2 = 0x04; // OP_STORE',
        '@0xF3 = 0x10; // Pulse lane',
        'pulse _>_;',
        'emit _;',
    ],
);

my constant %SIGNAL_KIND = (
    '^'    => 'high',
    '_'    => 'low',
    '?'    => 'uncertain',
    '_>_'  => 'falling',
    '_>>_' => 'double-falling',
);

sub parse-line(Str $line, NanLineActions $actions --> Hash) {
    my $match = NanLineGrammar.parse($line, :$actions);
    if !$match || !$match.made.defined {
        return { type => 'invalid' };
    }
    return $match.made;
}

sub lint-pulse-flow(
    Str $signal,
    Int $line-no,
    Str $origin,
    Str $last-pulse is rw,
    @errors
) {
    my $sig = $signal.trim;
    return unless %SIGNAL_KIND{$sig}:exists;

    my $kind = %SIGNAL_KIND{$sig};
    if $kind eq 'uncertain' {
        @errors.push("[pulse-flow] line {$line-no}: `pulse ?;` is not allowed {$origin}.");
        return;
    }

    if $kind eq 'falling' || $kind eq 'double-falling' {
        if $last-pulse ne '^' {
            @errors.push("[pulse-flow] line {$line-no}: {$sig} requires previous pulse state `^` {$origin}.");
        }
        $last-pulse = '_';
        return;
    }

    $last-pulse = $sig;
}

sub MAIN(Str $source-file) {
    my $actions = NanLineActions.new;

    my $fh = try {
        open $source-file, :r, :enc<utf8>;
    };

    if !$fh.defined {
        note "[preprocess] could not read source file: {$source-file}";
        exit 1;
    }

    my @output-lines;
    my @errors;
    my $line-no = 0;
    my $saw-nan3 = False;
    my $seen-code-before-magic = False;
    my $last-pulse = '';

    for $fh.lines -> $raw-line {
        $line-no++;
        my $line = $raw-line.subst(/\r$/, '');
        my $node = parse-line($line, $actions);

        given $node<type> {
            when 'blank' | 'comment' {
                @output-lines.push($line);
            }

            when 'magic' {
                if $seen-code-before-magic {
                    @errors.push("[magic] line {$line-no}: NAN3 must appear before source statements.");
                }
                $saw-nan3 = True;
            }

            when 'macro' {
                $seen-code-before-magic = True;
                my $macro-name = $node<name>.uc;

                unless %MACRO_EXPANSIONS{$macro-name}:exists {
                    my @known = %MACRO_EXPANSIONS.keys.sort;
                    @errors.push(
                        "[macro] line {$line-no}: unknown macro `{$macro-name}`. Known: {@known.join(', ')}."
                    );
                    next;
                }

                @output-lines.push("// expanded from \@macro({$macro-name}) at line {$line-no}");
                for %MACRO_EXPANSIONS{$macro-name}.List -> $expanded-line {
                    my $expanded-node = parse-line($expanded-line, $actions);
                    if $expanded-node<type> eq 'pulse' {
                        lint-pulse-flow(
                            $expanded-node<signal>,
                            $line-no,
                            '(expanded macro)',
                            $last-pulse,
                            @errors,
                        );
                    }
                    @output-lines.push($expanded-line);
                }
            }

            when 'pulse' {
                $seen-code-before-magic = True;
                lint-pulse-flow($node<signal>, $line-no, '(source)', $last-pulse, @errors);
                @output-lines.push($line);
            }

            when 'emit' | 'jitter' | 'halt' {
                $seen-code-before-magic = True;
                @output-lines.push($line);
            }

            when 'macro-invalid' {
                @errors.push("[syntax] line {$line-no}: malformed macro invocation; use \@macro(NAME).");
            }

            when 'pulse-invalid' {
                @errors.push("[syntax] line {$line-no}: malformed pulse statement; expected `pulse <expr>;`.");
            }

            when 'emit-invalid' {
                @errors.push("[syntax] line {$line-no}: malformed emit statement; expected `emit <expr>;`.");
            }

            default {
                if $line.trim ne '' {
                    $seen-code-before-magic = True;
                }
                @output-lines.push($line);
            }
        }
    }

    if @errors {
        for @errors -> $err {
            note $err;
        }
        exit 1;
    }

    if $saw-nan3 {
        @output-lines.unshift('// NAN3 header acknowledged: compiler backend emits binary magic bytes.');
    }

    say @output-lines.join("\n");
}
