<?php
/**
* Smarty Internal Plugin Configfilelexer
*
* This is the lexer to break the config file source into tokens
* @package Smarty
* @subpackage Config
* @author Uwe Tews
*/
/**
* Smarty Internal Plugin Configfilelexer
*/
class Smarty_Internal_Configfilelexer
{

    public $data;
    public $counter;
    public $token;
    public $value;
    public $node;
    public $line;
    private $state = 1;
    public $smarty_token_names = array (		// Text for parser error messages
   				);


    function __construct($data, $smarty)
    {
        // set instance object
        self::instance($this);
        $this->data = $data . "\n"; //now all lines are \n-terminated
        $this->counter = 0;
        $this->line = 1;
        $this->smarty = $smarty;
        $this->mbstring_overload = ini_get('mbstring.func_overload') & 2;
    }
    public static function &instance($new_instance = null)
    {
        static $instance = null;
        if (isset($new_instance) && is_object($new_instance))
            $instance = $new_instance;
        return $instance;
    }



    private $_yy_state = 1;
    private $_yy_stack = array();

    function yylex()
    {
        return $this->{'yylex' . $this->_yy_state}();
    }

    function yypushstate($state)
    {
        array_push($this->_yy_stack, $this->_yy_state);
        $this->_yy_state = $state;
    }

    function yypopstate()
    {
        $this->_yy_state = array_pop($this->_yy_stack);
    }

    function yybegin($state)
    {
        $this->_yy_state = $state;
    }




    function yylex1()
    {
        $tokenMap = array (
              1 => 0,
              2 => 0,
              3 => 0,
              4 => 0,
              5 => 0,
              6 => 0,
              7 => 0,
              8 => 0,
            );
        if ($this->counter >= ($this->mbstring_overload ? mb_strlen($this->data,'latin1'): strlen($this->data))) {
            return false; // end of input
        }
        $yy_global_pattern = "/\G(#|;)|\G(\\[)|\G(\\])|\G(=)|\G([ \t\r]+)|\G(\n)|\G([0-9]*[a-zA-Z_]\\w*)|\G([\S\s])/iS";

        do {
            if ($this->mbstring_overload ? preg_match($yy_global_pattern, mb_substr($this->data, $this->counter,2000000000,'latin1'), $yymatches) : preg_match($yy_global_pattern,$this->data, $yymatches, null, $this->counter)) {
                $yysubmatches = $yymatches;
                $yymatches = array_filter($yymatches, 'strlen'); // remove empty sub-patterns
                if (!count($yymatches)) {
                    throw new Exception('Error: lexing failed because a rule matched' .
                        ' an empty string.  Input "' . substr($this->data,
                        $this->counter, 5) . '... state START');
                }
                next($yymatches); // skip global match
                $this->token = key($yymatches); // token number
                if ($tokenMap[$this->token]) {
                    // extract sub-patterns for passing to lex function
                    $yysubmatches = array_slice($yysubmatches, $this->token + 1,
                        $tokenMap[$this->token]);
                } else {
                    $yysubmatches = array();
                }
                $this->value = current($yymatches); // token value
                $r = $this->{'yy_r1_' . $this->token}($yysubmatches);
                if ($r === null) {
                    $this->counter += ($this->mbstring_overload ? mb_strlen($this->value,'latin1'): strlen($this->value));
                    $this->line += substr_count($this->value, "\n");
                    // accept this token
                    return true;
                } elseif ($r === true) {
                    // we have changed state
                    // process this token in the new state
                    return $this->yylex();
                } elseif ($r === false) {
                    $this->counter += ($this->mbstring_overload ? mb_strlen($this->value,'latin1'): strlen($this->value));
                    $this->line += substr_count($this->value, "\n");
                    if ($this->counter >= ($this->mbstring_overload ? mb_strlen($this->data,'latin1'): strlen($this->data))) {
                        return false; // end of input
                    }
                    // skip this token
                    continue;
                }            } else {
                throw new Exception('Unexpected input at line' . $this->line .
                    ': ' . $this->data[$this->counter]);
            }
            break;
        } while (true);

    } // end function


    const START = 1;
    function yy_r1_1($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_COMMENTSTART;
    $this->yypushstate(self::COMMENT);
    }
    function yy_r1_2($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_OPENB;
    $this->yypushstate(self::SECTION);
    }
    function yy_r1_3($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_CLOSEB;
    }
    function yy_r1_4($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_EQUAL;
    $this->yypushstate(self::VALUE);
    }
    function yy_r1_5($yy_subpatterns)
    {

    return false;
    }
    function yy_r1_6($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_NEWLINE;
    }
    function yy_r1_7($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_ID;
    }
    function yy_r1_8($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_OTHER;
    }



    function yylex2()
    {
        $tokenMap = array (
              1 => 0,
              2 => 0,
              3 => 0,
              4 => 0,
              5 => 0,
              6 => 0,
              7 => 0,
              8 => 0,
              9 => 0,
            );
        if ($this->counter >= ($this->mbstring_overload ? mb_strlen($this->data,'latin1'): strlen($this->data))) {
            return false; // end of input
        }
        $yy_global_pattern = "/\G([ \t\r]+)|\G(\\d+\\.\\d+(?=[ \t\r]*[\n#;]))|\G(\\d+(?=[ \t\r]*[\n#;]))|\G(\"\"\")|\G('[^'\\\\]*(?:\\\\.[^'\\\\]*)*'(?=[ \t\r]*[\n#;]))|\G(\"[^\"\\\\]*(?:\\\\.[^\"\\\\]*)*\"(?=[ \t\r]*[\n#;]))|\G([a-zA-Z]+(?=[ \t\r]*[\n#;]))|\G([^\n]+?(?=[ \t\r]*\n))|\G(\n)/iS";

        do {
            if ($this->mbstring_overload ? preg_match($yy_global_pattern, mb_substr($this->data, $this->counter,2000000000,'latin1'), $yymatches) : preg_match($yy_global_pattern,$this->data, $yymatches, null, $this->counter)) {
                $yysubmatches = $yymatches;
                $yymatches = array_filter($yymatches, 'strlen'); // remove empty sub-patterns
                if (!count($yymatches)) {
                    throw new Exception('Error: lexing failed because a rule matched' .
                        ' an empty string.  Input "' . substr($this->data,
                        $this->counter, 5) . '... state VALUE');
                }
                next($yymatches); // skip global match
                $this->token = key($yymatches); // token number
                if ($tokenMap[$this->token]) {
                    // extract sub-patterns for passing to lex function
                    $yysubmatches = array_slice($yysubmatches, $this->token + 1,
                        $tokenMap[$this->token]);
                } else {
                    $yysubmatches = array();
                }
                $this->value = current($yymatches); // token value
                $r = $this->{'yy_r2_' . $this->token}($yysubmatches);
                if ($r === null) {
                    $this->counter += ($this->mbstring_overload ? mb_strlen($this->value,'latin1'): strlen($this->value));
                    $this->line += substr_count($this->value, "\n");
                    // accept this token
                    return true;
                } elseif ($r === true) {
                    // we have changed state
                    // process this token in the new state
                    return $this->yylex();
                } elseif ($r === false) {
                    $this->counter += ($this->mbstring_overload ? mb_strlen($this->value,'latin1'): strlen($this->value));
                    $this->line += substr_count($this->value, "\n");
                    if ($this->counter >= ($this->mbstring_overload ? mb_strlen($this->data,'latin1'): strlen($this->data))) {
                        return false; // end of input
                    }
                    // skip this token
                    continue;
                }            } else {
                throw new Exception('Unexpected input at line' . $this->line .
                    ': ' . $this->data[$this->counter]);
            }
            break;
        } while (true);

    } // end function


    const VALUE = 2;
    function yy_r2_1($yy_subpatterns)
    {

    return false;
    }
    function yy_r2_2($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_FLOAT;
    $this->yypopstate();
    }
    function yy_r2_3($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_INT;
    $this->yypopstate();
    }
    function yy_r2_4($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_TRIPPLE_QUOTES;
    $this->yypushstate(self::TRIPPLE);
    }
    function yy_r2_5($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_SINGLE_QUOTED_STRING;
    $this->yypopstate();
    }
    function yy_r2_6($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_DOUBLE_QUOTED_STRING;
    $this->yypopstate();
    }
    function yy_r2_7($yy_subpatterns)
    {

    if (!$this->smarty->config_booleanize || !in_array(strtolower($this->value), Array("true", "false", "on", "off", "yes", "no")) ) {
        $this->yypopstate();
        $this->yypushstate(self::NAKED_STRING_VALUE);
        return true; //reprocess in new state
    } else {
        $this->token = Smarty_Internal_Configfileparser::TPC_BOOL;
        $this->yypopstate();
    }
    }
    function yy_r2_8($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_NAKED_STRING;
    $this->yypopstate();
    }
    function yy_r2_9($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_NAKED_STRING;
    $this->value = "";
    $this->yypopstate();
    }



    function yylex3()
    {
        $tokenMap = array (
              1 => 0,
            );
        if ($this->counter >= ($this->mbstring_overload ? mb_strlen($this->data,'latin1'): strlen($this->data))) {
            return false; // end of input
        }
        $yy_global_pattern = "/\G([^\n]+?(?=[ \t\r]*\n))/iS";

        do {
            if ($this->mbstring_overload ? preg_match($yy_global_pattern, mb_substr($this->data, $this->counter,2000000000,'latin1'), $yymatches) : preg_match($yy_global_pattern,$this->data, $yymatches, null, $this->counter)) {
                $yysubmatches = $yymatches;
                $yymatches = array_filter($yymatches, 'strlen'); // remove empty sub-patterns
                if (!count($yymatches)) {
                    throw new Exception('Error: lexing failed because a rule matched' .
                        ' an empty string.  Input "' . substr($this->data,
                        $this->counter, 5) . '... state NAKED_STRING_VALUE');
                }
                next($yymatches); // skip global match
                $this->token = key($yymatches); // token number
                if ($tokenMap[$this->token]) {
                    // extract sub-patterns for passing to lex function
                    $yysubmatches = array_slice($yysubmatches, $this->token + 1,
                        $tokenMap[$this->token]);
                } else {
                    $yysubmatches = array();
                }
                $this->value = current($yymatches); // token value
                $r = $this->{'yy_r3_' . $this->token}($yysubmatches);
                if ($r === null) {
                    $this->counter += ($this->mbstring_overload ? mb_strlen($this->value,'latin1'): strlen($this->value));
                    $this->line += substr_count($this->value, "\n");
                    // accept this token
                    return true;
                } elseif ($r === true) {
                    // we have changed state
                    // process this token in the new state
                    return $this->yylex();
                } elseif ($r === false) {
                    $this->counter += ($this->mbstring_overload ? mb_strlen($this->value,'latin1'): strlen($this->value));
                    $this->line += substr_count($this->value, "\n");
                    if ($this->counter >= ($this->mbstring_overload ? mb_strlen($this->data,'latin1'): strlen($this->data))) {
                        return false; // end of input
                    }
                    // skip this token
                    continue;
                }            } else {
                throw new Exception('Unexpected input at line' . $this->line .
                    ': ' . $this->data[$this->counter]);
            }
            break;
        } while (true);

    } // end function


    const NAKED_STRING_VALUE = 3;
    function yy_r3_1($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_NAKED_STRING;
    $this->yypopstate();
    }



    function yylex4()
    {
        $tokenMap = array (
              1 => 0,
              2 => 0,
              3 => 0,
            );
        if ($this->counter >= ($this->mbstring_overload ? mb_strlen($this->data,'latin1'): strlen($this->data))) {
            return false; // end of input
        }
        $yy_global_pattern = "/\G([ \t\r]+)|\G([^\n]+?(?=[ \t\r]*\n))|\G(\n)/iS";

        do {
            if ($this->mbstring_overload ? preg_match($yy_global_pattern, mb_substr($this->data, $this->counter,2000000000,'latin1'), $yymatches) : preg_match($yy_global_pattern,$this->data, $yymatches, null, $this->counter)) {
                $yysubmatches = $yymatches;
                $yymatches = array_filter($yymatches, 'strlen'); // remove empty sub-patterns
                if (!count($yymatches)) {
                    throw new Exception('Error: lexing failed because a rule matched' .
                        ' an empty string.  Input "' . substr($this->data,
                        $this->counter, 5) . '... state COMMENT');
                }
                next($yymatches); // skip global match
                $this->token = key($yymatches); // token number
                if ($tokenMap[$this->token]) {
                    // extract sub-patterns for passing to lex function
                    $yysubmatches = array_slice($yysubmatches, $this->token + 1,
                        $tokenMap[$this->token]);
                } else {
                    $yysubmatches = array();
                }
                $this->value = current($yymatches); // token value
                $r = $this->{'yy_r4_' . $this->token}($yysubmatches);
                if ($r === null) {
                    $this->counter += ($this->mbstring_overload ? mb_strlen($this->value,'latin1'): strlen($this->value));
                    $this->line += substr_count($this->value, "\n");
                    // accept this token
                    return true;
                } elseif ($r === true) {
                    // we have changed state
                    // process this token in the new state
                    return $this->yylex();
                } elseif ($r === false) {
                    $this->counter += ($this->mbstring_overload ? mb_strlen($this->value,'latin1'): strlen($this->value));
                    $this->line += substr_count($this->value, "\n");
                    if ($this->counter >= ($this->mbstring_overload ? mb_strlen($this->data,'latin1'): strlen($this->data))) {
                        return false; // end of input
                    }
                    // skip this token
                    continue;
                }            } else {
                throw new Exception('Unexpected input at line' . $this->line .
                    ': ' . $this->data[$this->counter]);
            }
            break;
        } while (true);

    } // end function


    const COMMENT = 4;
    function yy_r4_1($yy_subpatterns)
    {

    return false;
    }
    function yy_r4_2($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_NAKED_STRING;
    }
    function yy_r4_3($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_NEWLINE;
    $this->yypopstate();
    }



    function yylex5()
    {
        $tokenMap = array (
              1 => 0,
              2 => 0,
            );
        if ($this->counter >= ($this->mbstring_overload ? mb_strlen($this->data,'latin1'): strlen($this->data))) {
            return false; // end of input
        }
        $yy_global_pattern = "/\G(\\.)|\G(.*?(?=[\.=[\]\r\n]))/iS";

        do {
            if ($this->mbstring_overload ? preg_match($yy_global_pattern, mb_substr($this->data, $this->counter,2000000000,'latin1'), $yymatches) : preg_match($yy_global_pattern,$this->data, $yymatches, null, $this->counter)) {
                $yysubmatches = $yymatches;
                $yymatches = array_filter($yymatches, 'strlen'); // remove empty sub-patterns
                if (!count($yymatches)) {
                    throw new Exception('Error: lexing failed because a rule matched' .
                        ' an empty string.  Input "' . substr($this->data,
                        $this->counter, 5) . '... state SECTION');
                }
                next($yymatches); // skip global match
                $this->token = key($yymatches); // token number
                if ($tokenMap[$this->token]) {
                    // extract sub-patterns for passing to lex function
                    $yysubmatches = array_slice($yysubmatches, $this->token + 1,
                        $tokenMap[$this->token]);
                } else {
                    $yysubmatches = array();
                }
                $this->value = current($yymatches); // token value
                $r = $this->{'yy_r5_' . $this->token}($yysubmatches);
                if ($r === null) {
                    $this->counter += ($this->mbstring_overload ? mb_strlen($this->value,'latin1'): strlen($this->value));
                    $this->line += substr_count($this->value, "\n");
                    // accept this token
                    return true;
                } elseif ($r === true) {
                    // we have changed state
                    // process this token in the new state
                    return $this->yylex();
                } elseif ($r === false) {
                    $this->counter += ($this->mbstring_overload ? mb_strlen($this->value,'latin1'): strlen($this->value));
                    $this->line += substr_count($this->value, "\n");
                    if ($this->counter >= ($this->mbstring_overload ? mb_strlen($this->data,'latin1'): strlen($this->data))) {
                        return false; // end of input
                    }
                    // skip this token
                    continue;
                }            } else {
                throw new Exception('Unexpected input at line' . $this->line .
                    ': ' . $this->data[$this->counter]);
            }
            break;
        } while (true);

    } // end function


    const SECTION = 5;
    function yy_r5_1($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_DOT;
    }
    function yy_r5_2($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_SECTION;
    $this->yypopstate();
    }


    function yylex6()
    {
        $tokenMap = array (
              1 => 0,
              2 => 0,
            );
        if ($this->counter >= ($this->mbstring_overload ? mb_strlen($this->data,'latin1'): strlen($this->data))) {
            return false; // end of input
        }
        $yy_global_pattern = "/\G(\"\"\"(?=[ \t\r]*[\n#;]))|\G([\S\s])/iS";

        do {
            if ($this->mbstring_overload ? preg_match($yy_global_pattern, mb_substr($this->data, $this->counter,2000000000,'latin1'), $yymatches) : preg_match($yy_global_pattern,$this->data, $yymatches, null, $this->counter)) {
                $yysubmatches = $yymatches;
                $yymatches = array_filter($yymatches, 'strlen'); // remove empty sub-patterns
                if (!count($yymatches)) {
                    throw new Exception('Error: lexing failed because a rule matched' .
                        ' an empty string.  Input "' . substr($this->data,
                        $this->counter, 5) . '... state TRIPPLE');
                }
                next($yymatches); // skip global match
                $this->token = key($yymatches); // token number
                if ($tokenMap[$this->token]) {
                    // extract sub-patterns for passing to lex function
                    $yysubmatches = array_slice($yysubmatches, $this->token + 1,
                        $tokenMap[$this->token]);
                } else {
                    $yysubmatches = array();
                }
                $this->value = current($yymatches); // token value
                $r = $this->{'yy_r6_' . $this->token}($yysubmatches);
                if ($r === null) {
                    $this->counter += ($this->mbstring_overload ? mb_strlen($this->value,'latin1'): strlen($this->value));
                    $this->line += substr_count($this->value, "\n");
                    // accept this token
                    return true;
                } elseif ($r === true) {
                    // we have changed state
                    // process this token in the new state
                    return $this->yylex();
                } elseif ($r === false) {
                    $this->counter += ($this->mbstring_overload ? mb_strlen($this->value,'latin1'): strlen($this->value));
                    $this->line += substr_count($this->value, "\n");
                    if ($this->counter >= ($this->mbstring_overload ? mb_strlen($this->data,'latin1'): strlen($this->data))) {
                        return false; // end of input
                    }
                    // skip this token
                    continue;
                }            } else {
                throw new Exception('Unexpected input at line' . $this->line .
                    ': ' . $this->data[$this->counter]);
            }
            break;
        } while (true);

    } // end function


    const TRIPPLE = 6;
    function yy_r6_1($yy_subpatterns)
    {

    $this->token = Smarty_Internal_Configfileparser::TPC_TRIPPLE_QUOTES_END;
    $this->yypopstate();
    $this->yypushstate(self::START);
    }
    function yy_r6_2($yy_subpatterns)
    {

  if ($this->mbstring_overload) {
    $to = mb_strlen($this->data,'latin1');
  } else {
    $to = strlen($this->data);
  }
  preg_match("/\"\"\"[ \t\r]*[\n#;]/",$this->data,$match,PREG_OFFSET_CAPTURE,$this->counter);
  if (isset($match[0][1])) {
    $to = $match[0][1];
  } else {
    $this->compiler->trigger_template_error ("missing or misspelled literal closing tag");
  }
  if ($this->mbstring_overload) {
    $this->value = mb_substr($this->data,$this->counter,$to-$this->counter,'latin1');
  } else {
    $this->value = substr($this->data,$this->counter,$to-$this->counter);
  }
  $this->token = Smarty_Internal_Configfileparser::TPC_TRIPPLE_TEXT;
    }


}
?>