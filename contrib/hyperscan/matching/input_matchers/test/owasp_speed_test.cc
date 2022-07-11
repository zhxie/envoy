// Note: this should be run with --compilation_mode=opt, and would benefit from
// a quiescent system with disabled cstate power management.

#include "source/common/common/assert.h"
#include "source/common/common/regex.h"
#include "source/common/thread_local/thread_local_impl.h"

#include "benchmark/benchmark.h"
#include "contrib/hyperscan/matching/input_matchers/source/matcher.h"
#include "re2/set.h"

namespace Envoy {

const std::string clusterInput(size_t size) { return std::string(size, 'a'); }

const std::vector<std::string>& clusterRePatterns() {
  CONSTRUCT_ON_FIRST_USE(
      std::vector<std::string>,
      // clang-format off
      {"[\\r\\n]\\W*?(?:content-(?:type|length)|set-cookie|location):\\s*\\w",
       "(?:\\bhttp/\\d|<(?:html|meta)\\b)",
       "^[^:\\(\\)\\&\\|\\!\\<\\>\\~]*\\)\\s*(?:\\((?:[^,\\(\\)\\=\\&\\|\\!\\<\\>\\~]+[><~]?=|\\s*[&!|]\\s*(?:\\)|\\()?\\s*)|\\)\\s*\\(\\s*[\\&\\|\\!]\\s*|[&!|]\\s*\\([^\\(\\)\\=\\&\\|\\!\\<\\>\\~]+[><~]?=[^:\\(\\)\\&\\|\\!\\<\\>\\~]*)",
       "(?i)(?:;|\\{|\\||\\|\\||&|&&|\\n|\\r|`)\\s*[\\(,@\\'\"\\s]*(?:[\\w'\"\\./]+/|[\\x5c'\"\\^]*\\w[\\x5c'\"\\^]*:.*\\x5c|[\\^\\.\\w '\"/\\x5c]*\\x5c)?[\"\\^]*(?:m[\"\\^]*(?:y[\"\\^]*s[\"\\^]*q[\"\\^]*l(?:[\"\\^]*(?:d[\"\\^]*u[\"\\^]*m[\"\\^]*p(?:[\"\\^]*s[\"\\^]*l[\"\\^]*o[\"\\^]*w)?|h[\"\\^]*o[\"\\^]*t[\"\\^]*c[\"\\^]*o[\"\\^]*p[\"\\^]*y|a[\"\\^]*d[\"\\^]*m[\"\\^]*i[\"\\^]*n|s[\"\\^]*h[\"\\^]*o[\"\\^]*w))?|s[\"\\^]*(?:i[\"\\^]*(?:n[\"\\^]*f[\"\\^]*o[\"\\^]*3[\"\\^]*2|e[\"\\^]*x[\"\\^]*e[\"\\^]*c)|c[\"\\^]*o[\"\\^]*n[\"\\^]*f[\"\\^]*i[\"\\^]*g|g[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|t[\"\\^]*s[\"\\^]*c)|o[\"\\^]*(?:u[\"\\^]*n[\"\\^]*t[\"\\^]*(?:(?:[\\s,;]|\\.|/|<|>).*|v[\"\\^]*o[\"\\^]*l)|v[\"\\^]*e[\"\\^]*u[\"\\^]*s[\"\\^]*e[\"\\^]*r|[dr][\"\\^]*e[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*)|k[\"\\^]*(?:d[\"\\^]*i[\"\\^]*r[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|l[\"\\^]*i[\"\\^]*n[\"\\^]*k)|d[\"\\^]*(?:s[\"\\^]*c[\"\\^]*h[\"\\^]*e[\"\\^]*d|(?:[\\s,;]|\\.|/|<|>).*)|a[\"\\^]*p[\"\\^]*i[\"\\^]*s[\"\\^]*e[\"\\^]*n[\"\\^]*d|b[\"\\^]*s[\"\\^]*a[\"\\^]*c[\"\\^]*l[\"\\^]*i|e[\"\\^]*a[\"\\^]*s[\"\\^]*u[\"\\^]*r[\"\\^]*e|m[\"\\^]*s[\"\\^]*y[\"\\^]*s)|d[\"\\^]*(?:i[\"\\^]*(?:s[\"\\^]*k[\"\\^]*(?:(?:m[\"\\^]*g[\"\\^]*m|p[\"\\^]*a[\"\\^]*r)[\"\\^]*t|s[\"\\^]*h[\"\\^]*a[\"\\^]*d[\"\\^]*o[\"\\^]*w)|r[\"\\^]*(?:(?:[\\s,;]|\\.|/|<|>).*|u[\"\\^]*s[\"\\^]*e)|f[\"\\^]*f[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*)|e[\"\\^]*(?:l[\"\\^]*(?:p[\"\\^]*r[\"\\^]*o[\"\\^]*f|t[\"\\^]*r[\"\\^]*e[\"\\^]*e|(?:[\\s,;]|\\.|/|<|>).*)|v[\"\\^]*(?:m[\"\\^]*g[\"\\^]*m[\"\\^]*t|c[\"\\^]*o[\"\\^]*n)|(?:f[\"\\^]*r[\"\\^]*a|b[\"\\^]*u)[\"\\^]*g)|s[\"\\^]*(?:a[\"\\^]*(?:c[\"\\^]*l[\"\\^]*s|d[\"\\^]*d)|q[\"\\^]*u[\"\\^]*e[\"\\^]*r[\"\\^]*y|m[\"\\^]*o[\"\\^]*(?:v[\"\\^]*e|d)|g[\"\\^]*e[\"\\^]*t|r[\"\\^]*m)|(?:r[\"\\^]*i[\"\\^]*v[\"\\^]*e[\"\\^]*r[\"\\^]*q[\"\\^]*u[\"\\^]*e[\"\\^]*r|o[\"\\^]*s[\"\\^]*k[\"\\^]*e)[\"\\^]*y|(?:c[\"\\^]*o[\"\\^]*m[\"\\^]*c[\"\\^]*n[\"\\^]*f|x[\"\\^]*d[\"\\^]*i[\"\\^]*a)[\"\\^]*g|a[\"\\^]*t[\"\\^]*e[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|n[\"\\^]*s[\"\\^]*s[\"\\^]*t[\"\\^]*a[\"\\^]*t)|c[\"\\^]*(?:o[\"\\^]*(?:m[\"\\^]*(?:p[\"\\^]*(?:(?:a[\"\\^]*c[\"\\^]*t[\"\\^]*)?(?:[\\s,;]|\\.|/|<|>).*|m[\"\\^]*g[\"\\^]*m[\"\\^]*t)|e[\"\\^]*x[\"\\^]*p)|n[\"\\^]*(?:2[\"\\^]*p|v[\"\\^]*e)[\"\\^]*r[\"\\^]*t|p[\"\\^]*y)|l[\"\\^]*(?:e[\"\\^]*a[\"\\^]*(?:n[\"\\^]*m[\"\\^]*g[\"\\^]*r|r[\"\\^]*m[\"\\^]*e[\"\\^]*m)|u[\"\\^]*s[\"\\^]*t[\"\\^]*e[\"\\^]*r)|h[\"\\^]*(?:k[\"\\^]*(?:n[\"\\^]*t[\"\\^]*f[\"\\^]*s|d[\"\\^]*s[\"\\^]*k)|d[\"\\^]*i[\"\\^]*r[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*)|s[\"\\^]*(?:c[\"\\^]*(?:r[\"\\^]*i[\"\\^]*p[\"\\^]*t|c[\"\\^]*m[\"\\^]*d)|v[\"\\^]*d[\"\\^]*e)|e[\"\\^]*r[\"\\^]*t[\"\\^]*(?:u[\"\\^]*t[\"\\^]*i[\"\\^]*l|r[\"\\^]*e[\"\\^]*q)|a[\"\\^]*(?:l[\"\\^]*l[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|c[\"\\^]*l[\"\\^]*s)|m[\"\\^]*d(?:[\"\\^]*k[\"\\^]*e[\"\\^]*y)?|i[\"\\^]*p[\"\\^]*h[\"\\^]*e[\"\\^]*r|u[\"\\^]*r[\"\\^]*l)|f[\"\\^]*(?:o[\"\\^]*r[\"\\^]*(?:m[\"\\^]*a[\"\\^]*t[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|f[\"\\^]*i[\"\\^]*l[\"\\^]*e[\"\\^]*s|e[\"\\^]*a[\"\\^]*c[\"\\^]*h)|i[\"\\^]*n[\"\\^]*d[\"\\^]*(?:(?:[\\s,;]|\\.|/|<|>).*|s[\"\\^]*t[\"\\^]*r)|s[\"\\^]*(?:m[\"\\^]*g[\"\\^]*m[\"\\^]*t|u[\"\\^]*t[\"\\^]*i[\"\\^]*l)|t[\"\\^]*(?:p[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|y[\"\\^]*p[\"\\^]*e)|r[\"\\^]*e[\"\\^]*e[\"\\^]*d[\"\\^]*i[\"\\^]*s[\"\\^]*k|c[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|g[\"\\^]*r[\"\\^]*e[\"\\^]*p)|n[\"\\^]*(?:e[\"\\^]*t[\"\\^]*(?:s[\"\\^]*(?:t[\"\\^]*a[\"\\^]*t|v[\"\\^]*c|h)|(?:[\\s,;]|\\.|/|<|>).*|c[\"\\^]*a[\"\\^]*t|d[\"\\^]*o[\"\\^]*m)|t[\"\\^]*(?:b[\"\\^]*a[\"\\^]*c[\"\\^]*k[\"\\^]*u[\"\\^]*p|r[\"\\^]*i[\"\\^]*g[\"\\^]*h[\"\\^]*t[\"\\^]*s)|(?:s[\"\\^]*l[\"\\^]*o[\"\\^]*o[\"\\^]*k[\"\\^]*u|m[\"\\^]*a)[\"\\^]*p|c[\"\\^]*(?:(?:[\\s,;]|\\.|/|<|>).*|a[\"\\^]*t)|b[\"\\^]*t[\"\\^]*s[\"\\^]*t[\"\\^]*a[\"\\^]*t)|e[\"\\^]*(?:x[\"\\^]*p[\"\\^]*(?:a[\"\\^]*n[\"\\^]*d[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|l[\"\\^]*o[\"\\^]*r[\"\\^]*e[\"\\^]*r)|v[\"\\^]*e[\"\\^]*n[\"\\^]*t[\"\\^]*(?:c[\"\\^]*r[\"\\^]*e[\"\\^]*a[\"\\^]*t[\"\\^]*e|v[\"\\^]*w[\"\\^]*r)|n[\"\\^]*d[\"\\^]*l[\"\\^]*o[\"\\^]*c[\"\\^]*a[\"\\^]*l|g[\"\\^]*r[\"\\^]*e[\"\\^]*p|r[\"\\^]*a[\"\\^]*s[\"\\^]*e|c[\"\\^]*h[\"\\^]*o)|g[\"\\^]*(?:a[\"\\^]*t[\"\\^]*h[\"\\^]*e[\"\\^]*r[\"\\^]*n[\"\\^]*e[\"\\^]*t[\"\\^]*w[\"\\^]*o[\"\\^]*r[\"\\^]*k[\"\\^]*i[\"\\^]*n[\"\\^]*f[\"\\^]*o|p[\"\\^]*(?:(?:r[\"\\^]*e[\"\\^]*s[\"\\^]*u[\"\\^]*l|e[\"\\^]*d[\"\\^]*i)[\"\\^]*t|u[\"\\^]*p[\"\\^]*d[\"\\^]*a[\"\\^]*t[\"\\^]*e)|i[\"\\^]*t[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|e[\"\\^]*t[\"\\^]*m[\"\\^]*a[\"\\^]*c)|i[\"\\^]*(?:r[\"\\^]*b(?:[\"\\^]*(?:1(?:[\"\\^]*[89])?|2[\"\\^]*[012]))?|f[\"\\^]*m[\"\\^]*e[\"\\^]*m[\"\\^]*b[\"\\^]*e[\"\\^]*r|p[\"\\^]*c[\"\\^]*o[\"\\^]*n[\"\\^]*f[\"\\^]*i[\"\\^]*g|n[\"\\^]*e[\"\\^]*t[\"\\^]*c[\"\\^]*p[\"\\^]*l|c[\"\\^]*a[\"\\^]*c[\"\\^]*l[\"\\^]*s)|a[\"\\^]*(?:d[\"\\^]*(?:d[\"\\^]*u[\"\\^]*s[\"\\^]*e[\"\\^]*r[\"\\^]*s|m[\"\\^]*o[\"\\^]*d[\"\\^]*c[\"\\^]*m[\"\\^]*d)|r[\"\\^]*p[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|t[\"\\^]*t[\"\\^]*r[\"\\^]*i[\"\\^]*b|s[\"\\^]*s[\"\\^]*o[\"\\^]*c|z[\"\\^]*m[\"\\^]*a[\"\\^]*n)|l[\"\\^]*(?:o[\"\\^]*g[\"\\^]*(?:e[\"\\^]*v[\"\\^]*e[\"\\^]*n[\"\\^]*t|t[\"\\^]*i[\"\\^]*m[\"\\^]*e|m[\"\\^]*a[\"\\^]*n|o[\"\\^]*f[\"\\^]*f)|a[\"\\^]*b[\"\\^]*e[\"\\^]*l[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|u[\"\\^]*s[\"\\^]*r[\"\\^]*m[\"\\^]*g[\"\\^]*r)|b[\"\\^]*(?:(?:c[\"\\^]*d[\"\\^]*(?:b[\"\\^]*o[\"\\^]*o|e[\"\\^]*d[\"\\^]*i)|r[\"\\^]*o[\"\\^]*w[\"\\^]*s[\"\\^]*t[\"\\^]*a)[\"\\^]*t|i[\"\\^]*t[\"\\^]*s[\"\\^]*a[\"\\^]*d[\"\\^]*m[\"\\^]*i[\"\\^]*n|o[\"\\^]*o[\"\\^]*t[\"\\^]*c[\"\\^]*f[\"\\^]*g)|h[\"\\^]*(?:o[\"\\^]*s[\"\\^]*t[\"\\^]*n[\"\\^]*a[\"\\^]*m[\"\\^]*e|d[\"\\^]*w[\"\\^]*w[\"\\^]*i[\"\\^]*z)|j[\"\\^]*a[\"\\^]*v[\"\\^]*a[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|7[\"\\^]*z(?:[\"\\^]*[ar])?)(?:\\.[\"\\^]*\\w+)?\\b",
       "(?i)(?:;|\\{|\\||\\|\\||&|&&|\\n|\\r|`)\\s*[\\(,@\\'\"\\s]*(?:[\\w'\"\\./]+/|[\\x5c'\"\\^]*\\w[\\x5c'\"\\^]*:.*\\x5c|[\\^\\.\\w '\"/\\x5c]*\\x5c)?[\"\\^]*(?:s[\"\\^]*(?:y[\"\\^]*s[\"\\^]*(?:t[\"\\^]*e[\"\\^]*m[\"\\^]*(?:p[\"\\^]*r[\"\\^]*o[\"\\^]*p[\"\\^]*e[\"\\^]*r[\"\\^]*t[\"\\^]*i[\"\\^]*e[\"\\^]*s[\"\\^]*(?:d[\"\\^]*a[\"\\^]*t[\"\\^]*a[\"\\^]*e[\"\\^]*x[\"\\^]*e[\"\\^]*c[\"\\^]*u[\"\\^]*t[\"\\^]*i[\"\\^]*o[\"\\^]*n[\"\\^]*p[\"\\^]*r[\"\\^]*e[\"\\^]*v[\"\\^]*e[\"\\^]*n[\"\\^]*t[\"\\^]*i[\"\\^]*o[\"\\^]*n|(?:p[\"\\^]*e[\"\\^]*r[\"\\^]*f[\"\\^]*o[\"\\^]*r[\"\\^]*m[\"\\^]*a[\"\\^]*n[\"\\^]*c|h[\"\\^]*a[\"\\^]*r[\"\\^]*d[\"\\^]*w[\"\\^]*a[\"\\^]*r)[\"\\^]*e|a[\"\\^]*d[\"\\^]*v[\"\\^]*a[\"\\^]*n[\"\\^]*c[\"\\^]*e[\"\\^]*d)|i[\"\\^]*n[\"\\^]*f[\"\\^]*o)|k[\"\\^]*e[\"\\^]*y|d[\"\\^]*m)|h[\"\\^]*(?:o[\"\\^]*(?:w[\"\\^]*(?:g[\"\\^]*r[\"\\^]*p|m[\"\\^]*b[\"\\^]*r)[\"\\^]*s|r[\"\\^]*t[\"\\^]*c[\"\\^]*u[\"\\^]*t)|e[\"\\^]*l[\"\\^]*l[\"\\^]*r[\"\\^]*u[\"\\^]*n[\"\\^]*a[\"\\^]*s|u[\"\\^]*t[\"\\^]*d[\"\\^]*o[\"\\^]*w[\"\\^]*n|r[\"\\^]*p[\"\\^]*u[\"\\^]*b[\"\\^]*w|a[\"\\^]*r[\"\\^]*e|i[\"\\^]*f[\"\\^]*t)|e[\"\\^]*(?:t[\"\\^]*(?:(?:x[\"\\^]*)?(?:[\\s,;]|\\.|/|<|>).*|l[\"\\^]*o[\"\\^]*c[\"\\^]*a[\"\\^]*l)|c[\"\\^]*p[\"\\^]*o[\"\\^]*l|l[\"\\^]*e[\"\\^]*c[\"\\^]*t)|c[\"\\^]*(?:h[\"\\^]*t[\"\\^]*a[\"\\^]*s[\"\\^]*k[\"\\^]*s|l[\"\\^]*i[\"\\^]*s[\"\\^]*t)|u[\"\\^]*b[\"\\^]*(?:i[\"\\^]*n[\"\\^]*a[\"\\^]*c[\"\\^]*l|s[\"\\^]*t)|(?:t[\"\\^]*a|o)[\"\\^]*r[\"\\^]*t[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|i[\"\\^]*g[\"\\^]*v[\"\\^]*e[\"\\^]*r[\"\\^]*i[\"\\^]*f|l[\"\\^]*(?:e[\"\\^]*e[\"\\^]*p|m[\"\\^]*g[\"\\^]*r)|f[\"\\^]*c|v[\"\\^]*n)|p[\"\\^]*(?:s[\"\\^]*(?:s[\"\\^]*(?:h[\"\\^]*u[\"\\^]*t[\"\\^]*d[\"\\^]*o[\"\\^]*w[\"\\^]*n|e[\"\\^]*r[\"\\^]*v[\"\\^]*i[\"\\^]*c[\"\\^]*e|u[\"\\^]*s[\"\\^]*p[\"\\^]*e[\"\\^]*n[\"\\^]*d)|l[\"\\^]*(?:o[\"\\^]*g[\"\\^]*(?:g[\"\\^]*e[\"\\^]*d[\"\\^]*o[\"\\^]*n|l[\"\\^]*i[\"\\^]*s[\"\\^]*t)|i[\"\\^]*s[\"\\^]*t)|p[\"\\^]*(?:a[\"\\^]*s[\"\\^]*s[\"\\^]*w[\"\\^]*d|i[\"\\^]*n[\"\\^]*g)|g[\"\\^]*e[\"\\^]*t[\"\\^]*s[\"\\^]*i[\"\\^]*d|e[\"\\^]*x[\"\\^]*e[\"\\^]*c|f[\"\\^]*i[\"\\^]*l[\"\\^]*e|i[\"\\^]*n[\"\\^]*f[\"\\^]*o|k[\"\\^]*i[\"\\^]*l[\"\\^]*l)|o[\"\\^]*(?:w[\"\\^]*e[\"\\^]*r[\"\\^]*(?:s[\"\\^]*h[\"\\^]*e[\"\\^]*l[\"\\^]*l(?:[\"\\^]*_[\"\\^]*i[\"\\^]*s[\"\\^]*e)?|c[\"\\^]*f[\"\\^]*g)|r[\"\\^]*t[\"\\^]*q[\"\\^]*r[\"\\^]*y|p[\"\\^]*d)|r[\"\\^]*(?:i[\"\\^]*n[\"\\^]*t[\"\\^]*(?:(?:[\\s,;]|\\.|/|<|>).*|b[\"\\^]*r[\"\\^]*m)|n[\"\\^]*(?:c[\"\\^]*n[\"\\^]*f[\"\\^]*g|m[\"\\^]*n[\"\\^]*g[\"\\^]*r)|o[\"\\^]*m[\"\\^]*p[\"\\^]*t)|a[\"\\^]*t[\"\\^]*h[\"\\^]*(?:p[\"\\^]*i[\"\\^]*n[\"\\^]*g|(?:[\\s,;]|\\.|/|<|>).*)|e[\"\\^]*r[\"\\^]*(?:l(?:[\"\\^]*(?:s[\"\\^]*h|5))?|f[\"\\^]*m[\"\\^]*o[\"\\^]*n)|y[\"\\^]*t[\"\\^]*h[\"\\^]*o[\"\\^]*n(?:[\"\\^]*(?:3(?:[\"\\^]*m)?|2))?|k[\"\\^]*g[\"\\^]*m[\"\\^]*g[\"\\^]*r|h[\"\\^]*p(?:[\"\\^]*[57])?|u[\"\\^]*s[\"\\^]*h[\"\\^]*d|i[\"\\^]*n[\"\\^]*g)|r[\"\\^]*(?:e[\"\\^]*(?:(?:p[\"\\^]*l[\"\\^]*a[\"\\^]*c[\"\\^]*e|n(?:[\"\\^]*a[\"\\^]*m[\"\\^]*e)?|s[\"\\^]*e[\"\\^]*t)[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|g[\"\\^]*(?:s[\"\\^]*v[\"\\^]*r[\"\\^]*3[\"\\^]*2|e[\"\\^]*d[\"\\^]*i[\"\\^]*t|(?:[\\s,;]|\\.|/|<|>).*|i[\"\\^]*n[\"\\^]*i)|c[\"\\^]*(?:d[\"\\^]*i[\"\\^]*s[\"\\^]*c|o[\"\\^]*v[\"\\^]*e[\"\\^]*r)|k[\"\\^]*e[\"\\^]*y[\"\\^]*w[\"\\^]*i[\"\\^]*z)|u[\"\\^]*(?:n[\"\\^]*(?:d[\"\\^]*l[\"\\^]*l[\"\\^]*3[\"\\^]*2|a[\"\\^]*s)|b[\"\\^]*y[\"\\^]*(?:1(?:[\"\\^]*[89])?|2[\"\\^]*[012]))|a[\"\\^]*(?:s[\"\\^]*(?:p[\"\\^]*h[\"\\^]*o[\"\\^]*n[\"\\^]*e|d[\"\\^]*i[\"\\^]*a[\"\\^]*l)|r[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*)|m[\"\\^]*(?:(?:d[\"\\^]*i[\"\\^]*r[\"\\^]*)?(?:[\\s,;]|\\.|/|<|>).*|t[\"\\^]*s[\"\\^]*h[\"\\^]*a[\"\\^]*r[\"\\^]*e)|o[\"\\^]*(?:u[\"\\^]*t[\"\\^]*e[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|b[\"\\^]*o[\"\\^]*c[\"\\^]*o[\"\\^]*p[\"\\^]*y)|s[\"\\^]*(?:t[\"\\^]*r[\"\\^]*u[\"\\^]*i|y[\"\\^]*n[\"\\^]*c)|d[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*)|t[\"\\^]*(?:a[\"\\^]*(?:s[\"\\^]*k[\"\\^]*(?:k[\"\\^]*i[\"\\^]*l[\"\\^]*l|l[\"\\^]*i[\"\\^]*s[\"\\^]*t|s[\"\\^]*c[\"\\^]*h[\"\\^]*d|m[\"\\^]*g[\"\\^]*r)|k[\"\\^]*e[\"\\^]*o[\"\\^]*w[\"\\^]*n)|(?:i[\"\\^]*m[\"\\^]*e[\"\\^]*o[\"\\^]*u|p[\"\\^]*m[\"\\^]*i[\"\\^]*n[\"\\^]*i|e[\"\\^]*l[\"\\^]*n[\"\\^]*e|l[\"\\^]*i[\"\\^]*s)[\"\\^]*t|s[\"\\^]*(?:d[\"\\^]*i[\"\\^]*s[\"\\^]*c[\"\\^]*o|s[\"\\^]*h[\"\\^]*u[\"\\^]*t[\"\\^]*d)[\"\\^]*n|y[\"\\^]*p[\"\\^]*e[\"\\^]*(?:p[\"\\^]*e[\"\\^]*r[\"\\^]*f|(?:[\\s,;]|\\.|/|<|>).*)|r[\"\\^]*(?:a[\"\\^]*c[\"\\^]*e[\"\\^]*r[\"\\^]*t|e[\"\\^]*e))|w[\"\\^]*(?:i[\"\\^]*n[\"\\^]*(?:d[\"\\^]*i[\"\\^]*f[\"\\^]*f|m[\"\\^]*s[\"\\^]*d[\"\\^]*p|v[\"\\^]*a[\"\\^]*r|r[\"\\^]*[ms])|u[\"\\^]*(?:a[\"\\^]*(?:u[\"\\^]*c[\"\\^]*l[\"\\^]*t|p[\"\\^]*p)|s[\"\\^]*a)|s[\"\\^]*c[\"\\^]*(?:r[\"\\^]*i[\"\\^]*p[\"\\^]*t|u[\"\\^]*i)|e[\"\\^]*v[\"\\^]*t[\"\\^]*u[\"\\^]*t[\"\\^]*i[\"\\^]*l|m[\"\\^]*i[\"\\^]*(?:m[\"\\^]*g[\"\\^]*m[\"\\^]*t|c)|a[\"\\^]*i[\"\\^]*t[\"\\^]*f[\"\\^]*o[\"\\^]*r|h[\"\\^]*o[\"\\^]*a[\"\\^]*m[\"\\^]*i|g[\"\\^]*e[\"\\^]*t)|u[\"\\^]*(?:s[\"\\^]*(?:e[\"\\^]*r[\"\\^]*a[\"\\^]*c[\"\\^]*c[\"\\^]*o[\"\\^]*u[\"\\^]*n[\"\\^]*t[\"\\^]*c[\"\\^]*o[\"\\^]*n[\"\\^]*t[\"\\^]*r[\"\\^]*o[\"\\^]*l[\"\\^]*s[\"\\^]*e[\"\\^]*t[\"\\^]*t[\"\\^]*i[\"\\^]*n[\"\\^]*g[\"\\^]*s|r[\"\\^]*s[\"\\^]*t[\"\\^]*a[\"\\^]*t)|n[\"\\^]*(?:r[\"\\^]*a[\"\\^]*r|z[\"\\^]*i[\"\\^]*p))|q[\"\\^]*(?:u[\"\\^]*e[\"\\^]*r[\"\\^]*y[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|p[\"\\^]*r[\"\\^]*o[\"\\^]*c[\"\\^]*e[\"\\^]*s[\"\\^]*s|w[\"\\^]*i[\"\\^]*n[\"\\^]*s[\"\\^]*t[\"\\^]*a|g[\"\\^]*r[\"\\^]*e[\"\\^]*p)|o[\"\\^]*(?:d[\"\\^]*b[\"\\^]*c[\"\\^]*(?:a[\"\\^]*d[\"\\^]*3[\"\\^]*2|c[\"\\^]*o[\"\\^]*n[\"\\^]*f)|p[\"\\^]*e[\"\\^]*n[\"\\^]*f[\"\\^]*i[\"\\^]*l[\"\\^]*e[\"\\^]*s)|v[\"\\^]*(?:o[\"\\^]*l[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*|e[\"\\^]*r[\"\\^]*i[\"\\^]*f[\"\\^]*y)|x[\"\\^]*c[\"\\^]*(?:a[\"\\^]*c[\"\\^]*l[\"\\^]*s|o[\"\\^]*p[\"\\^]*y)|z[\"\\^]*i[\"\\^]*p[\"\\^]*(?:[\\s,;]|\\.|/|<|>).*)(?:\\.[\"\\^]*\\w+)?\\b",
       "(?:\\$(?:\\((?:\\(.*\\)|.*)\\)|\\{.*})|[<>]\\(.*\\))",
       "\\b(?:if(?:/i)?(?: not)?(?: exist\\b| defined\\b| errorlevel\\b| cmdextversion\\b|(?: |\\().*(?:\\bgeq\\b|\\bequ\\b|\\bneq\\b|\\bleq\\b|\\bgtr\\b|\\blss\\b|==))|for(?:/[dflr].*)? %+[^ ]+ in\\(.*\\)\\s?do)",
       "(?:;|\\{|\\||\\|\\||&|&&|\\n|\\r|\\$\\(|\\$\\(\\(|`|\\${|<\\(|>\\(|\\(\\s*\\))\\s*(?:{|\\s*\\(\\s*|\\w+=(?:[^\\s]*|\\$.*|\\$.*|<.*|>.*|\\'.*\\'|\".*\")\\s+|!\\s*|\\$)*\\s*(?:'|\")*(?:[\\?\\*\\[\\]\\(\\)\\-\\|+\\w'\"\\./\\x5c]+/)?[\\x5c'\"]*\\.\\s.*\\b",
       ";\\s*\\.\\s*(?:s(?:e(?:parator|lftest)|c(?:anstats|hema)|h(?:a3sum|ell|ow)|ystem|tats|ave)|t(?:estc(?:ase|trl)|ime(?:out|r)|ables|race)|e(?:x(?:p(?:lain|ert)|cel|it)|cho|qp)|p(?:r(?:o(?:gress|mpt)|int)|arameter)|d(?:b(?:config|info)|atabases|ump)|c(?:h(?:anges|eck)|lone|d)|i(?:mpo(?:ster|rt)|ndexes)|l(?:i(?:mi|n)t|o(?:ad|g))|b(?:a(?:ckup|il)|inary)|f(?:ullschema|ilectrl)|vfs(?:info|list|name)|re(?:cover|store|ad)|o(?:utput|nce|pen)|(?:nullvalu|mod)e|a(?:rchive|uth)|he(?:aders|lp)|width|quit)",
       "(?is)\\r\\n\\w{1,50}\\b[ ](?:A(?:PPEND (?:[\\w\"\\.\\-\\x5c\\/%\\*&#]+)?(?: \\((?:[a-z\\x5c\\ ])+\\))?(?: \"?\\d{1,2}-\\w{3}-\\d{4} \\d{2}:\\d{2}:\\d{2} [+-]\\d{4}\"?)? \\{\\d{1,20}\\+?}|UTHENTICATE [a-z0-9-_]{1,20}\\r\\n)|S(?:TATUS (?:[\\w\"\\.\\-\\x5c\\/%\\*&]+)? \\((?:UNSEEN|UIDNEXT|MESSAGES|UIDVALIDITY|RECENT| )+\\)|ETACL (?:[\\w\"\\.\\-\\x5c\\/%\\*&]+)? [+-][lrswipckdxtea]+?)|L(?:SUB (?:[\\w\"~\\/\\*#\\.]+)? (?:[\\w\"\\.\\x5c\\/%\\*&]+)?|ISTRIGHTS (?:[\\w\"\\.\\-\\x5c\\/%\\*&]+)?)|(?:(?:DELETE|GET)ACL|MYRIGHTS) (?:[\\w\"\\.\\-\\x5c\\/%\\*&]+)?|UID (?:COPY|FETCH|STORE) (?:[0-9,:\\*]+)?)",
       "(?si)\\r\\n.*?\\b(?:A(?:UTH [A-Z0-9-_]{1,20} (?:=|(?:[\\w+/]{4})*(?:[\\w+/]{2}==|[\\w+/]{3}=))|POP [\\w]+ [a-f0-9]{32})|(?:TOP \\d+|LIST)(?: \\d+)?|U(?:IDL(?: \\d+)?|SER .+?)|(?:DELE|RETR) \\d+?|PASS .+?)",
       "(?:;|\\{|\\||\\|\\||&|&&|\\n|\\r|\\$\\(|\\$\\(\\(|`|\\${|<\\(|>\\(|\\(\\s*\\))\\s*(?:{|\\s*\\(\\s*|\\w+=(?:[^\\s]*|\\$.*|\\$.*|<.*|>.*|\\'.*\\'|\".*\")\\s+|!\\s*|\\$)*\\s*(?:'|\")*(?:[\\?\\*\\[\\]\\(\\)\\-\\|+\\w'\"\\./\\x5c]+/)?[\\x5c'\"]*(?:(?:(?:a[\\x5c'\"]*p[\\x5c'\"]*t[\\x5c'\"]*i[\\x5c'\"]*t[\\x5c'\"]*u[\\x5c'\"]*d|u[\\x5c'\"]*p[\\x5c'\"]*2[\\x5c'\"]*d[\\x5c'\"]*a[\\x5c'\"]*t)[\\x5c'\"]*e|d[\\x5c'\"]*n[\\x5c'\"]*f|v[\\x5c'\"]*i)[\\x5c'\"]*(?:\\s|<|>).*|p[\\x5c'\"]*(?:a[\\x5c'\"]*c[\\x5c'\"]*m[\\x5c'\"]*a[\\x5c'\"]*n[\\x5c'\"]*(?:\\s|<|>).*|w[\\x5c'\"]*d|s)|w[\\x5c'\"]*(?:(?:\\s|<|>).*|h[\\x5c'\"]*o))\\b",
       "(?s)\\r\\n.*?\\b(?:HELP(?: .{1,255})?|DATA|QUIT)",
       "(?is)\\r\\n\\w{1,50}\\b[ ](?:S(?:E(?:ARCH(?: CHARSET [\\w\\-_\\.]{1,40})? (?:(KEYWORD \\x5c)?(?:ALL|ANSWERED|BCC|DELETED|DRAFT|FLAGGED|RECENT|SEEN|UNANSWERED|UNDELETED|UNDRAFT|UNFLAGGED|UNSEEN|NEW|OLD)|(?:TEXT .{1,255}|TO .{1,255}|UID [0-9,:\\*]+?|UNKEYWORD (?:\\x5c(Seen|Answered|Flagged|Deleted|Draft|Recent)))|(?:BEFORE|ON|SENTBEFORE|SENTON|SENTSINCE|SINCE) \"?\\d{1,2}-\\w{3}-\\d{4}\"?|(?:OR .{1,255} .{1,255}|SMALLER \\d{1,20}|SUBJECT .{1,255})|(?:(?:BODY|CC|FROM)|HEADER .{1,100}) .{1,255}|(?:LARGER \\d{1,20}|NOT .{1,255}|[0-9,:\\*]+))|LECT [\\w\"\\.\\-\\x5c\\/%\\*&#]+)|T(?:ORE [0-9,:\\*]+? [+-]?FLAGS(?:\\.SILENT)? (?:\\(\\x5c[a-z]{1,20}\\))?|ARTTLS)|UBSCRIBE [\\w\"\\.\\-\\x5c\\/%\\*&#]+)|L(?:IST [\\w\"~\\-\\x5c\\/\\*#\\.]+? [\\w\"\\.\\-\\x5c\\/%\\*&#]+|OG(?:IN [a-z0-9-_\\.\\@]{1,40} .*?|OUT))|C(?:(?:OPY [0-9,:\\*]+|REATE) [\\w\"\\.\\-\\x5c\\/%\\*&#]+|APABILITY|HECK|LOSE)|RENAME [\\w\"\\.\\-\\x5c\\/%\\*&#]+? [\\w\"\\.\\-\\x5c\\/%\\*&#]+|UN(?:SUBSCRIBE [\\w\"\\.\\-\\x5c\\/%\\*&#]+|AUTHENTICATE)|EX(?:AMINE [\\w\"\\.\\-\\x5c%\\*&#]+|PUNGE)|DELETE [\\w\"\\.\\-\\x5c%\\*&#]+|FETCH [0-9,:\\*]+|NOOP)",
       "(?si)\\r\\n.*?\\b(?:(?:QUI|RSE|STA)T|CAPA|NOOP)",
       "(?:<\\?(?:[^x]|x[^m]|xm[^l]|xml[^\\s]|xml$|$)|<\\?php|\\[(?:/|\\x5c)?php\\])",
       "(?i)php://(?:std(?:in|out|err)|(?:in|out)put|fd|memory|temp|filter)",
       "(?i:zlib|glob|phar|ssh2|rar|ogg|expect|zip)://",
       "(?i)\\b(?:s(?:e(?:t(?:_(?:e(?:xception|rror)_handler|magic_quotes_runtime|include_path)|defaultstub)|ssion_s(?:et_save_handler|tart))|qlite_(?:(?:(?:unbuffered|single|array)_)?query|create_(?:aggregate|function)|p?open|exec)|tr(?:eam_(?:context_create|socket_client)|ipc?slashes|rev)|implexml_load_(?:string|file)|ocket_c(?:onnect|reate)|h(?:ow_sourc|a1_fil)e|pl_autoload_register|ystem)|p(?:r(?:eg_(?:replace(?:_callback(?:_array)?)?|match(?:_all)?|split)|oc_(?:(?:terminat|clos|nic)e|get_status|open)|int_r)|o(?:six_(?:get(?:(?:e[gu]|g)id|login|pwnam)|mk(?:fifo|nod)|ttyname|kill)|pen)|hp(?:_(?:strip_whitespac|unam)e|version|info)|g_(?:(?:execut|prepar)e|connect|query)|a(?:rse_(?:ini_file|str)|ssthru)|utenv)|r(?:unkit_(?:function_(?:re(?:defin|nam)e|copy|add)|method_(?:re(?:defin|nam)e|copy|add)|constant_(?:redefine|add))|e(?:(?:gister_(?:shutdown|tick)|name)_function|ad(?:(?:gz)?file|_exif_data|dir))|awurl(?:de|en)code)|i(?:mage(?:createfrom(?:(?:jpe|pn)g|x[bp]m|wbmp|gif)|(?:jpe|pn)g|g(?:d2?|if)|2?wbmp|xbm)|s_(?:(?:(?:execut|write?|read)ab|fi)le|dir)|ni_(?:get(?:_all)?|set)|terator_apply|ptcembed)|g(?:et(?:_(?:c(?:urrent_use|fg_va)r|meta_tags)|my(?:[gpu]id|inode)|(?:lastmo|cw)d|imagesize|env)|z(?:(?:(?:defla|wri)t|encod|fil)e|compress|open|read)|lob)|a(?:rray_(?:u(?:intersect(?:_u?assoc)?|diff(?:_u?assoc)?)|intersect_u(?:assoc|key)|diff_u(?:assoc|key)|filter|reduce|map)|ssert(?:_options)?)|h(?:tml(?:specialchars(?:_decode)?|_entity_decode|entities)|(?:ash(?:_(?:update|hmac))?|ighlight)_file|e(?:ader_register_callback|x2bin))|f(?:i(?:le(?:(?:[acm]tim|inod)e|(?:_exist|perm)s|group)?|nfo_open)|tp_(?:nb_(?:ge|pu)|connec|ge|pu)t|(?:unction_exis|pu)ts|write|open)|o(?:b_(?:get_(?:c(?:ontents|lean)|flush)|end_(?:clean|flush)|clean|flush|start)|dbc_(?:result(?:_all)?|exec(?:ute)?|connect)|pendir)|m(?:b_(?:ereg(?:_(?:replace(?:_callback)?|match)|i(?:_replace)?)?|parse_str)|(?:ove_uploaded|d5)_file|ethod_exists|ysql_query|kdir)|e(?:x(?:if_(?:t(?:humbnail|agname)|imagetype|read_data)|ec)|scapeshell(?:arg|cmd)|rror_reporting|val)|c(?:url_(?:file_create|exec|init)|onvert_uuencode|reate_function|hr)|u(?:n(?:serialize|pack)|rl(?:de|en)code|[ak]?sort)|(?:json_(?:de|en)cod|debug_backtrac|tmpfil)e|b(?:(?:son_(?:de|en)|ase64_en)code|zopen)|var_dump)(?:\\s|/\\*.*\\*/|//.*|#.*)*\\(.*\\)",
       "[oOcC]:\\d+:\".+?\":\\d+:{.*}",
       "\\$+(?:[a-zA-Z_\\x7f-\\xff][a-zA-Z0-9_\\x7f-\\xff]*|\\s*{.+})(?:\\s|\\[.+\\]|{.+}|/\\*.*\\*/|//.*|#.*)*\\(.*\\)",
       "(\\(.+\\)\\(.+\\)|\\(.+\\)['\"][a-zA-Z-_0-9]+['\"]\\(.+\\)|\\[\\d+\\]\\(.+\\)|\\{\\d+\\}\\(.+\\)|\\$[^(\\),.;\\x5c/]+\\(.+\\)|[\"'][a-zA-Z0-9-_\\x5c]+[\"']\\(.+\\)|\\([^\\)]*string[^\\)]*\\)[a-zA-Z-_0-9\"'.{}\\[\\]\\s]+\\([^\\)]*\\));",
       "(?:HTTP_(?:ACCEPT(?:_(?:ENCODING|LANGUAGE|CHARSET))?|(?:X_FORWARDED_FO|REFERE)R|(?:USER_AGEN|HOS)T|CONNECTION|KEEP_ALIVE)|PATH_(?:TRANSLATED|INFO)|ORIG_PATH_INFO|QUERY_STRING|REQUEST_URI|AUTH_TYPE)",
       "(?i)\\b(?:i(?:s(?:_(?:in(?:t(?:eger)?|finite)|n(?:u(?:meric|ll)|an)|(?:calla|dou)ble|s(?:calar|tring)|f(?:inite|loat)|re(?:source|al)|l(?:ink|ong)|a(?:rray)?|object|bool)|set)|n(?:(?:clud|vok)e|t(?:div|val))|(?:mplod|dat)e|conv)|s(?:t(?:r(?:(?:le|sp)n|coll)|at)|(?:e(?:rializ|ttyp)|huffl)e|i(?:milar_text|zeof|nh?)|p(?:liti?|rintf)|(?:candi|ubst)r|y(?:mlink|slog)|o(?:undex|rt)|leep|rand|qrt)|f(?:ile(?:(?:siz|typ)e|owner|pro)|l(?:o(?:atval|ck|or)|ush)|(?:rea|mo)d|t(?:ell|ok)|unction|close|gets|stat|eof)|c(?:h(?:o(?:wn|p)|eckdate|root|dir|mod)|o(?:(?:(?:nsta|u)n|mpac)t|sh?|py)|lose(?:dir|log)|(?:urren|ryp)t|eil)|e(?:x(?:(?:trac|i)t|p(?:lode)?)|a(?:ster_da(?:te|ys)|ch)|r(?:ror_log|egi?)|mpty|cho|nd)|l(?:o(?:g(?:1[0p])?|caltime)|i(?:nk(?:info)?|st)|(?:cfirs|sta)t|evenshtein|trim)|d(?:i(?:(?:skfreespac)?e|r(?:name)?)|e(?:fined?|coct)|(?:oubleva)?l|ate)|r(?:e(?:(?:quir|cod|nam)e|adlin[ek]|wind|set)|an(?:ge|d)|ound|sort|trim)|m(?:b(?:split|ereg)|i(?:crotime|n)|a(?:i[ln]|x)|etaphone|y?sql|hash)|u(?:n(?:(?:tain|se)t|iqid|link)|s(?:leep|ort)|cfirst|mask)|a(?:s(?:(?:se|o)rt|inh?)|r(?:sort|ray)|tan[2h]?|cosh?|bs)|t(?:e(?:xtdomain|mpnam)|a(?:int|nh?)|ouch|ime|rim)|h(?:e(?:ader(?:s_(?:lis|sen)t)?|brev)|ypot|ash)|p(?:a(?:thinfo|ck)|r(?:intf?|ev)|close|o[sw]|i)|g(?:et(?:t(?:ext|ype)|date)|mdate)|o(?:penlog|ctdec|rd)|b(?:asename|indec)|n(?:atsor|ex)t|k(?:sort|ey)|quotemeta|wordwrap|virtual|join)(?:\\s|/\\*.*\\*/|//.*|#.*)*\\(.*\\)",
       "(?:_(?:\\$\\$ND_FUNC\\$\\$_|_js_function)|(?:new\\s+Function|\\beval)\\s*\\(|String\\s*\\.\\s*fromCharCode|function\\s*\\(\\s*\\)\\s*{|module\\.exports\\s*=|this\\.constructor)",
       "(?:__proto__|constructor\\s*(?:\\.|\\[)\\s*prototype)",
       "\\[\\s*constructor\\s*\\]",
       "(?i)<script[^>]*>[\\s\\S]*?",
       "(?i)[\\s\\S](?:\\b(?:x(?:link:href|html|mlns)|data:text\\/html|pattern\\b.*?=|formaction)|!ENTITY\\s+(?:\\S+|%\\s+\\S+)\\s+(?:PUBLIC|SYSTEM)|;base64|@import)\\b",
       "(?i)[a-z]+=(?:[^:=]+:.+;)*?[^:=]+:url\\(javascript",
       "(?i)(?:(?:<\\w[\\s\\S]*[\\s/]|['\"](?:[\\s\\S]*[\\s/])?)(?:on(?:d(?:e(?:vice(?:(?:orienta|mo)tion|proximity|found|light)|livery(?:success|error)|activate)|r(?:ag(?:e(?:n(?:ter|d)|xit)|(?:gestur|leav)e|start|drop|over)|op)|i(?:s(?:c(?:hargingtimechange|onnect(?:ing|ed))|abled)|aling)|ata(?:setc(?:omplete|hanged)|(?:availabl|chang)e|error)|urationchange|ownloading|blclick)|Moz(?:M(?:agnifyGesture(?:Update|Start)?|ouse(?:PixelScroll|Hittest))|S(?:wipeGesture(?:Update|Start|End)?|crolledAreaChanged)|(?:(?:Press)?TapGestur|BeforeResiz)e|EdgeUI(?:C(?:omplet|ancel)|Start)ed|RotateGesture(?:Update|Start)?|A(?:udioAvailable|fterPaint))|c(?:o(?:m(?:p(?:osition(?:update|start|end)|lete)|mand(?:update)?)|n(?:t(?:rolselect|extmenu)|nect(?:ing|ed))|py)|a(?:(?:llschang|ch)ed|nplay(?:through)?|rdstatechange)|h(?:(?:arging(?:time)?ch)?ange|ecking)|(?:fstate|ell)change|u(?:echange|t)|l(?:ick|ose))|s(?:t(?:a(?:t(?:uschanged|echange)|lled|rt)|k(?:sessione|comma)nd|op)|e(?:ek(?:complete|ing|ed)|(?:lec(?:tstar)?)?t|n(?:ding|t))|(?:peech|ound)(?:start|end)|u(?:ccess|spend|bmit)|croll|how)|m(?:o(?:z(?:(?:pointerlock|fullscreen)(?:change|error)|(?:orientation|time)change|network(?:down|up)load)|use(?:(?:lea|mo)ve|o(?:ver|ut)|enter|wheel|down|up)|ve(?:start|end)?)|essage|ark)|a(?:n(?:imation(?:iteration|start|end)|tennastatechange)|fter(?:(?:scriptexecu|upda)te|print)|udio(?:process|start|end)|d(?:apteradded|dtrack)|ctivate|lerting|bort)|b(?:e(?:fore(?:(?:(?:de)?activa|scriptexecu)te|u(?:nload|pdate)|p(?:aste|rint)|c(?:opy|ut)|editfocus)|gin(?:Event)?)|oun(?:dary|ce)|l(?:ocked|ur)|roadcast|usy)|DOM(?:Node(?:Inserted(?:IntoDocument)?|Removed(?:FromDocument)?)|(?:CharacterData|Subtree)Modified|A(?:ttrModified|ctivate)|Focus(?:Out|In)|MouseScroll)|r(?:e(?:s(?:u(?:m(?:ing|e)|lt)|ize|et)|adystatechange|pea(?:tEven)?t|movetrack|trieving|ceived)|ow(?:s(?:inserted|delete)|e(?:nter|xit))|atechange)|p(?:op(?:up(?:hid(?:den|ing)|show(?:ing|n))|state)|a(?:ge(?:hide|show)|(?:st|us)e|int)|ro(?:pertychange|gress)|lay(?:ing)?)|t(?:ouch(?:(?:lea|mo)ve|en(?:ter|d)|cancel|start)|ransition(?:cancel|end|run)|ime(?:update|out)|ext)|u(?:s(?:erproximity|sdreceived)|p(?:gradeneeded|dateready)|n(?:derflow|load))|f(?:o(?:rm(?:change|input)|cus(?:out|in)?)|i(?:lterchange|nish)|ailed)|l(?:o(?:ad(?:e(?:d(?:meta)?data|nd)|start)|secapture)|evelchange|y)|g(?:amepad(?:(?:dis)?connected|button(?:down|up)|axismove)|et)|e(?:n(?:d(?:Event|ed)?|abled|ter)|rror(?:update)?|mptied|xit)|i(?:cc(?:cardlockerror|infochange)|n(?:coming|valid|put))|o(?:(?:(?:ff|n)lin|bsolet)e|verflow(?:changed)?|pen)|SVG(?:(?:Unl|L)oad|Resize|Scroll|Abort|Error|Zoom)|h(?:e(?:adphoneschange|l[dp])|ashchange|olding)|v(?:o(?:lum|ic)e|ersion)change|w(?:a(?:it|rn)ing|heel)|key(?:press|down|up)|(?:AppComman|Loa)d|no(?:update|match)|Request|zoom)|s(?:tyle|rc)|background|formaction|lowsrc|ping)[\\s\\x08]*?=|<[^\\w<>]*(?:[^<>\"'\\s]*:)?[^\\w<>]*\\W*?(?:(?:a\\W*?(?:n\\W*?i\\W*?m\\W*?a\\W*?t\\W*?e|p\\W*?p\\W*?l\\W*?e\\W*?t|u\\W*?d\\W*?i\\W*?o)|b\\W*?(?:i\\W*?n\\W*?d\\W*?i\\W*?n\\W*?g\\W*?s|a\\W*?s\\W*?e|o\\W*?d\\W*?y)|i?\\W*?f\\W*?r\\W*?a\\W*?m\\W*?e|o\\W*?b\\W*?j\\W*?e\\W*?c\\W*?t|i\\W*?m\\W*?a?\\W*?g\\W*?e?|e\\W*?m\\W*?b\\W*?e\\W*?d|p\\W*?a\\W*?r\\W*?a\\W*?m|v\\W*?i\\W*?d\\W*?e\\W*?o|l\\W*?i\\W*?n\\W*?k)[^>\\w]|s\\W*?(?:c\\W*?r\\W*?i\\W*?p\\W*?t|t\\W*?y\\W*?l\\W*?e|e\\W*?t[^>\\w]|v\\W*?g)|m\\W*?(?:a\\W*?r\\W*?q\\W*?u\\W*?e\\W*?e|e\\W*?t\\W*?a[^>\\w])|f\\W*?o\\W*?r\\W*?m))",
       "(?i)(?:\\W|^)(?:javascript:(?:[\\s\\S]+[=\\x5c\\(\\[\\.<]|[\\s\\S]*?(?:\\bname\\b|\\x5c[ux]\\d))|data:(?:(?:[a-z]\\w+/\\w[\\w+-]+\\w)?[;,]|[\\s\\S]*?;[\\s\\S]*?\\b(?:base64|charset=)|[\\s\\S]*?,[\\s\\S]*?<[\\s\\S]*?\\w[\\s\\S]*?>))|@\\W*?i\\W*?m\\W*?p\\W*?o\\W*?r\\W*?t\\W*?(?:/\\*[\\s\\S]*?)?(?:[\"']|\\W*?u\\W*?r\\W*?l[\\s\\S]*?\\()|[^-]*?-\\W*?m\\W*?o\\W*?z\\W*?-\\W*?b\\W*?i\\W*?n\\W*?d\\W*?i\\W*?n\\W*?g[^:]*?:\\W*?u\\W*?r\\W*?l[\\s\\S]*?\\(",
       "(?i:<style.*?>.*?(?:@[i\\x5c]|(?:[:=]|&#x?0*(?:58|3A|61|3D);?).*?(?:[(\\x5c]|&#x?0*(?:40|28|92|5C);?)))",
       "(?i:<.*[:]?vmlframe.*?[\\s/+]*?src[\\s/+]*=)",
       "(?i:(?:j|&#x?0*(?:74|4A|106|6A);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?:a|&#x?0*(?:65|41|97|61);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?:v|&#x?0*(?:86|56|118|76);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?:a|&#x?0*(?:65|41|97|61);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?:s|&#x?0*(?:83|53|115|73);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?:c|&#x?0*(?:67|43|99|63);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?:r|&#x?0*(?:82|52|114|72);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?:i|&#x?0*(?:73|49|105|69);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?:p|&#x?0*(?:80|50|112|70);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?:t|&#x?0*(?:84|54|116|74);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?::|&(?:#x?0*(?:58|3A);?|colon;)).)",
       "(?i:(?:v|&#x?0*(?:86|56|118|76);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?:b|&#x?0*(?:66|42|98|62);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?:s|&#x?0*(?:83|53|115|73);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?:c|&#x?0*(?:67|43|99|63);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?:r|&#x?0*(?:82|52|114|72);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?:i|&#x?0*(?:73|49|105|69);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?:p|&#x?0*(?:80|50|112|70);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?:t|&#x?0*(?:84|54|116|74);?)(?:\\t|&(?:#x?0*(?:9|13|10|A|D);?|tab;|newline;))*(?::|&(?:#x?0*(?:58|3A);?|colon;)).)",
       "(?i)<EMBED[\\s/+].*?(?:src|type).*?=",
       "<[?]?import[\\s/+\\S]*?implementation[\\s/+]*?=",
       "(?i:<META[\\s/+].*?http-equiv[\\s/+]*=[\\s/+]*[\"'`]?(?:(?:c|&#x?0*(?:67|43|99|63);?)|(?:r|&#x?0*(?:82|52|114|72);?)|(?:s|&#x?0*(?:83|53|115|73);?)))",
       "(?i:<META[\\s/+].*?charset[\\s/+]*=)",
       "(?i)<LINK[\\s/+].*?href[\\s/+]*=",
       "(?i)<BASE[\\s/+].*?href[\\s/+]*=",
       "(?i)<APPLET[\\s/+>]",
       "(?i)<OBJECT[\\s/+].*?(?:type|codetype|classid|code|data)[\\s/+]*=",
       "\\+ADw-.*(?:\\+AD4-|>)|<.*\\+AD4-",
       "![!+ ]\\[\\]",
       "(?:self|document|this|top|window)\\s*(?:/\\*|[\\[)]).+?(?:\\]|\\*/)",
       "(?i)[\\s\"'`;\\/0-9=\\x0B\\x09\\x0C\\x3B\\x2C\\x28\\x3B]on[a-zA-Z]{3,25}[\\s\\x0B\\x09\\x0C\\x3B\\x2C\\x28\\x3B]*?=[^=]",
       "(?i)\\b(?:s(?:tyle|rc)|href)\\b[\\s\\S]*?=",
       "<(?:a|abbr|acronym|address|applet|area|audioscope|b|base|basefront|bdo|bgsound|big|blackface|blink|blockquote|body|bq|br|button|caption|center|cite|code|col|colgroup|comment|dd|del|dfn|dir|div|dl|dt|em|embed|fieldset|fn|font|form|frame|frameset|h1|head|hr|html|i|iframe|ilayer|img|input|ins|isindex|kdb|keygen|label|layer|legend|li|limittext|link|listing|map|marquee|menu|meta|multicol|nobr|noembed|noframes|noscript|nosmartquotes|object|ol|optgroup|option|p|param|plaintext|pre|q|rt|ruby|s|samp|script|select|server|shadow|sidebar|small|spacer|span|strike|strong|style|sub|sup|table|tbody|td|textarea|tfoot|th|thead|title|tr|tt|u|ul|var|wbr|xml|xmp)\\W",
       "(?i:[\"'][ ]*(?:[^a-z0-9~_:' ]|in).*?(?:(?:l|\\x5cu006C)(?:o|\\x5cu006F)(?:c|\\x5cu0063)(?:a|\\x5cu0061)(?:t|\\x5cu0074)(?:i|\\x5cu0069)(?:o|\\x5cu006F)(?:n|\\x5cu006E)|(?:n|\\x5cu006E)(?:a|\\x5cu0061)(?:m|\\x5cu006D)(?:e|\\x5cu0065)|(?:o|\\x5cu006F)(?:n|\\x5cu006E)(?:e|\\x5cu0065)(?:r|\\x5cu0072)(?:r|\\x5cu0072)(?:o|\\x5cu006F)(?:r|\\x5cu0072)|(?:v|\\x5cu0076)(?:a|\\x5cu0061)(?:l|\\x5cu006C)(?:u|\\x5cu0075)(?:e|\\x5cu0065)(?:O|\\x5cu004F)(?:f|\\x5cu0066)).*?=)",
       "(?i)[\"\\'][ ]*(?:[^a-z0-9~_:\\' ]|in).+?[.].+?=",
       "{{.*?}}",
       "(?i)\\b(?:(?:m(?:s(?:ys(?:ac(?:cess(?:objects|storage|xml)|es)|(?:relationship|object|querie)s|modules2?)|db)|aster\\.\\.sysdatabases|ysql\\.db)|pg_(?:catalog|toast)|information_schema|northwind|tempdb)\\b|s(?:(?:ys(?:\\.database_name|aux)|qlite(?:_temp)?_master)\\b|chema(?:_name\\b|\\W*\\())|d(?:atabas|b_nam)e\\W*\\()",
       "(?i)\\b(?:c(?:o(?:n(?:v(?:ert(?:_tz)?)?|cat(?:_ws)?|nection_id)|(?:mpres)?s|ercibility|llation|alesce|t)|ur(?:rent_(?:time(?:stamp)?|date|user)|(?:dat|tim)e)|ha(?:racte)?r_length|iel(?:ing)?|r32)|s(?:t(?:d(?:dev_(?:sam|po)p)?|r(?:_to_date|cmp))|ub(?:str(?:ing(?:_index)?)?|(?:dat|tim)e)|e(?:ssion_user|c_to_time)|ys(?:tem_user|date)|ha[12]?|oundex|chema|pace|qrt|in)|i(?:s(?:_(?:ipv(?:4(?:_(?:compat|mapped))?|6)|n(?:ot(?:_null)?|ull)|(?:free|used)_lock)|null)|n(?:et(?:6_(?:aton|ntoa)|_(?:aton|ntoa))|s(?:ert|tr)|terval)|fnull)|d(?:a(?:t(?:e(?:_(?:format|add|sub)|diff)|abase)|y(?:of(?:month|week|year)|name))|e(?:s_(?:de|en)crypt|grees|code)|count|ump)|l(?:o(?:ca(?:ltimestamp|te)|g(?:10|2)|ad_file|wer)|ast_(?:inser_id|day)|e(?:as|f)t|inestring|case|trim|pad)|t(?:ime(?:_(?:format|to_sec)|stamp(?:diff|add)?|diff)|o(?:(?:second|day)s|_base64|n?char)|r(?:uncate|im))|u(?:n(?:compress(?:ed_length)?|ix_timestamp|hex)|tc_(?:time(?:stamp)?|date)|uid(?:_short)?|pdatexml|case)|m(?:a(?:ke(?:_set|date)|ster_pos_wait)|ulti(?:po(?:lygon|int)|linestring)|i(?:crosecon)?d|onthname|d5)|g(?:e(?:t_(?:format|lock)|ometrycollection)|(?:r(?:oup_conca|eates)|tid_subse)t)|p(?:o(?:(?:siti|lyg)on|w)|eriod_(?:diff|add)|rocedure_analyse|g_sleep)|a(?:s(?:cii(?:str)?|in)|es_(?:de|en)crypt|dd(?:dat|tim)e|tan2?)|f(?:rom_(?:unixtime|base64|days)|i(?:el|n)d_in_set|ound_rows)|e(?:x(?:tract(?:value)?|p(?:ort_set)?)|nc(?:rypt|ode)|lt)|b(?:i(?:t_(?:length|count|x?or|and)|n_to_num)|enchmark)|r(?:a(?:wtohex|dians|nd)|elease_lock|ow_count|trim|pad)|o(?:(?:ld_passwo)?rd|ct(?:et_length)?)|we(?:ek(?:ofyear|day)|ight_string)|n(?:ame_const|ot_in|ullif)|var(?:_(?:sam|po)p|iance)|qu(?:arter|ote)|hex(?:toraw)?|yearweek|xmltype)\\W*\\(",
       "(?i:sleep\\(\\s*?\\d*?\\s*?\\)|benchmark\\(.*?\\,.*?\\))",
       "(?i)(?:select|;)\\s+(?:benchmark|sleep|if)\\s*?\\(\\s*?\\(?\\s*?\\w+",
       "(?i)(?:\\b(?:u(?:nion(?:[\\w(\\s]*?select|\\sselect\\s@)|ser\\s*?\\([^\\)]*?)|(?:c(?:onnection_id|urrent_user)|database)\\s*?\\([^\\)]*?|s(?:chema\\s*?\\([^\\)]*?|elect.*?\\w?user\\()|into[\\s+]+(?:dump|out)file\\s*?[\"'`]|from\\W+information_schema\\W|exec(?:ute)?\\s+master\\.)|[\"'`](?:;?\\s*?(?:union\\b\\s*?(?:(?:distin|sele)ct|all)|having|select)\\b\\s*?[^\\s]|\\s*?!\\s*?[\"'`\\w])|\\s*?exec(?:ute)?.*?\\Wxp_cmdshell|\\Wiif\\s*?\\()",
       "^(?i:-0000023456|4294967295|4294967296|2147483648|2147483647|0000012345|-2147483648|-2147483649|0000023456|2.2250738585072007e-308|2.2250738585072011e-308|1e309)$",
       "(?i)(?:select.*?having\\s*?[^\\s]+\\s*?[^\\w\\s]|[\\s()]case\\s+when.*?then|if\\s?\\(\\w+\\s*?[=<>~]|\\)\\s*?like\\s*?\\()",
       "(?i)(?:[\"'`](?:;*?\\s*?waitfor\\s+(?:delay|time)\\s+[\"'`]|;.*?:\\s*?goto)|alter\\s*?\\w+.*?cha(?:racte)?r\\s+set\\s+\\w+)",
       "(?i:merge.*?using\\s*?\\(|execute\\s*?immediate\\s*?[\"'`]|match\\s*?[\\w(),+-]+\\s*?against\\s*?\\()",
       "(?i)union.*?select.*?from",
       "(?i)(?:;\\s*?shutdown\\s*?(?:[#;{]|\\/\\*|--)|waitfor\\s*?delay\\s?[\"'`]+\\s?\\d|select\\s*?pg_sleep)",
       "(?i:(?:\\[\\$(?:ne|eq|lte?|gte?|n?in|mod|all|size|exists|type|slice|x?or|div|like|between|and)\\]))",
       "(?i)(?:create\\s+(?:procedure|function)\\s*?\\w+\\s*?\\(\\s*?\\)\\s*?-|;\\s*?(?:declare|open)\\s+[\\w-]+|procedure\\s+analyse\\s*?\\(|declare[^\\w]+[@#]\\s*?\\w+|exec\\s*?\\(\\s*?@)",
       "(?i)(?:;\\s*?(?:(?:(?:trunc|cre|upd)at|renam)e|d(?:e(?:lete|sc)|rop)|(?:inser|selec)t|alter|load)\\b\\s*?[\\[(]?\\w{2,}|create\\s+function\\s.+\\sreturns)",
       "(?i)(?:^[\\W\\d]+\\s*?(?:alter\\s*(?:a(?:(?:pplication\\s*rol|ggregat)e|s(?:ymmetric\\s*ke|sembl)y|u(?:thorization|dit)|vailability\\s*group)|c(?:r(?:yptographic\\s*provider|edential)|o(?:l(?:latio|um)|nversio)n|ertificate|luster)|s(?:e(?:rv(?:ice|er)|curity|quence|ssion|arch)|y(?:mmetric\\s*key|nonym)|togroup|chema)|m(?:a(?:s(?:ter\\s*key|k)|terialized)|e(?:ssage\\s*type|thod)|odule)|l(?:o(?:g(?:file\\s*group|in)|ckdown)|a(?:ngua|r)ge|ibrary)|t(?:(?:abl(?:espac)?|yp)e|r(?:igger|usted)|hreshold|ext)|p(?:a(?:rtition|ckage)|ro(?:cedur|fil)e|ermission)|d(?:i(?:mension|skgroup)|atabase|efault|omain)|r(?:o(?:l(?:lback|e)|ute)|e(?:sourc|mot)e)|f(?:u(?:lltext|nction)|lashback|oreign)|e(?:xte(?:nsion|rnal)|(?:ndpoi|ve)nt)|in(?:dex(?:type)?|memory|stance)|b(?:roker\\s*priority|ufferpool)|x(?:ml\\s*schema|srobject)|w(?:ork(?:load)?|rapper)|hi(?:erarchy|stogram)|o(?:perator|utline)|(?:nicknam|queu)e|us(?:age|er)|group|java|view)\\b|(?:(?:(?:trunc|cre)at|renam)e|d(?:e(?:lete|sc)|rop)|(?:inser|selec)t|load)\\s+\\w+|u(?:nion\\s*(?:(?:distin|sele)ct|all)\\b|pdate\\s+\\w+))|\\b(?:(?:(?:(?:trunc|cre|upd)at|renam)e|(?:inser|selec)t|de(?:lete|sc)|alter|load)\\s+(?:group_concat|load_file|char)\\b\\s*\\(?|end\\s*?\\);)|[\"'`\\w]\\s+as\\b\\s*[\"'`\\w]+\\s*\\bfrom|[\\s(]load_file\\s*?\\(|[\"'`]\\s+regexp\\W)",
       "(?i:/\\*[!+](?:[\\w\\s=_\\-()]+)?\\*/)",
       "(?i)\\b(?:r(?:e(?:p(?:lace|eat)|verse)|ight|ound)|c(?:h(?:ar(?:set)?|r)|(?:oun|as)t)|m(?:in(?:ute)?|o(?:nth|d)|ax)|l(?:o(?:cal|g)|ength|ast|n)|(?:u(?:pp|s)e|hou|yea)r|s(?:econd|leep|ign|um)|d(?:a(?:te|y)|efault)|f(?:ormat|ield|loor)|p(?:assword|ower|i)|a(?:(?:co|b)s|vg)|v(?:ersion|alues)|t(?:ime|an)|i[fn]|week|bin|now)\\W*\\(",
       "(?i)(?:[\"'`](?:\\s*?(?:(?:between|x?or|and|div)[\\w\\s-]+\\s*?[+<>=(),-]\\s*?[\\d\"'`]|like(?:[\\w\\s-]+\\s*?[+<>=(),-]\\s*?[\\d\"'`]|\\W+[\\w\"'`(])|[!=|](?:[\\d\\s!=+-]+.*?[\"'`(].*?|[\\d\\s!=]+.*?\\d+)$|[^\\w\\s]?=\\s*?[\"'`])|(?:\\W*?[+=]+\\W*?|[<>~]+)[\"'`])|(?:/\\*)+[\"'`]+\\s?(?:[#{]|\\/\\*|--)?|\\d[\"'`]\\s+[\"'`]\\s+\\d|where\\s[\\s\\w\\.,-]+\\s=|^admin\\s*?[\"'`]|\\sis\\s*?0\\W)",
       "(?i)(?:(?:(?:(?:trunc|cre|upd)at|renam)e|d(?:e(?:lete|sc)|rop)|(?:inser|selec)t|alter|load)\\s*?\\(\\s*?space\\s*?\\(|,.*?[)\\da-f\"'`][\"'`](?:[\"'`].*?[\"'`]|(?:\\r?\\n)?\\z|[^\"'`]+)|\\Wselect.+\\W*?from)",
       "(?i)(?:(?:n(?:and|ot)|(?:x?x)?or|between|\\|\\||like|and|div|&&)[\\s(]+\\w+[\\s)]*?[!=+]+[\\s\\d]*?[\"'`=()]|\\d(?:\\s*?(?:between|like|x?or|and|div)\\s*?\\d+\\s*?[\\-+]|\\s+group\\s+by.+\\()|\\/\\w+;?\\s+(?:between|having|select|like|x?or|and|div)\\W|--\\s*?(?:(?:insert|update)\\s*?\\w{2,}|alter|drop)|#\\s*?(?:(?:insert|update)\\s*?\\w{2,}|alter|drop)|;\\s*?(?:(?:insert|update)\\s*?\\w{2,}|alter|drop)|@.+=\\s*?\\(\\s*?select|[^\\w]SET\\s*?@\\w+)",
       "(?i)(?:[\"'`]\\s*?(?:(?:n(?:and|ot)|(?:x?x)?or|between|\\|\\||and|div|&&)\\s+[\\s\\w]+=\\s*?\\w+\\s*?having\\s+|like(?:\\s+[\\s\\w]+=\\s*?\\w+\\s*?having\\s+|\\W*?[\"'`\\d]))|select\\s+?[\\[\\]()\\s\\w\\.,\"'`-]+from\\s+|\\w\\s+like\\s+[\"'`]|like\\s*?[\"'`]%)",
       "(?i)(?:\\b(?:(?:r(?:egexp|like)|n(?:and|ot)|(?:x?x)?or|like|and|div)\\s+\\s*?\\w+\\(|b(?:etween\\s+\\s*?\\w+\\(|inary\\s*?\\(\\s*?\\d)|cha?r\\s*?\\(\\s*?\\d)|\\)\\s*?when\\s*?\\d+\\s*?then|(?:\\|\\||&&)\\s+\\s*?\\w+\\(|[\"'`]\\s*?(?:[#{]|--)|\\/\\*!\\s?\\d+)",
       "(?i)(?:(?:\\(\\s*?select\\s*?\\w+|order\\s+by\\s+if\\w*?|coalesce)\\s*?\\(|[\"'`](?:;\\s*?(?:begin|while|if)|[\\s\\d]+=\\s*?\\d)|\\w[\"'`]\\s*?(?:(?:[-+=|@]+\\s+?)+|[-+=|@]+)[\\d(]|[\\s(]+case\\d*?\\W.+[tw]hen[\\s(]|\\+\\s*?\\d+\\s*?\\+\\s*?@|@@\\w+\\s*?[^\\w\\s]|\\W!+[\"'`]\\w|\\*\\/from)",
       "(?i)(?:^(?:[\"'`\\x5c]*?(?:[^\"'`]+[\"'`]|[\\d\"'`]+)\\s*?(?:n(?:and|ot)|(?:x?x)?or|between|\\|\\||like|and|div|&&)\\s*?[\\w\"'`][+&!@(),.-]|.?[\"'`]$)|@(?:[\\w-]+\\s(?:between|like|x?or|and|div)\\s*?[^\\w\\s]|\\w+\\s+(?:between|like|x?or|and|div)\\s*?[\"'`\\d]+)|[\"'`]\\s*?(?:between|like|x?or|and|div)\\s*?[\"'`]?\\d|[^\\w\\s:]\\s*?\\d\\W+[^\\w\\s]\\s*?[\"'`].|[^\\w\\s]\\w+\\s*?[|-]\\s*?[\"'`]\\s*?\\w|\\Winformation_schema|\\x5cx(?:23|27|3d)|table_name\\W)",
       "(?i)(?:[\"'`](?:\\s*?(?:is\\s*?(?:[\\d.]+\\s*?\\W.*?[\"'`]|\\d.+[\"'`]?\\w)|\\d\\s*?(?:--|#))|(?:\\W+[\\w+-]+\\s*?=\\s*?\\d\\W+|\\|?[\\w-]{3,}[^\\w\\s.,]+)[\"'`]|[\\%&<>^=]+\\d\\s*?(?:between|like|x?or|and|div|=))|(?i:n?and|x?x?or|div|like|between|not|\\|\\||\\&\\&)\\s+[\\s\\w+]+(?:sounds\\s+like\\s*?[\"'`]|regexp\\s*?\\(|[=\\d]+x)|in\\s*?\\(+\\s*?select)",
       "(?i:^[\\W\\d]+\\s*?(?:alter|union)\\b)",
       "(?i)(?:^[\\W\\d]+\\s*?(?:(?:alter\\s*(?:a(?:(?:pplication\\s*rol|ggregat)e|s(?:ymmetric\\s*ke|sembl)y|u(?:thorization|dit)|vailability\\s*group)|c(?:r(?:yptographic\\s*provider|edential)|o(?:l(?:latio|um)|nversio)n|ertificate|luster)|s(?:e(?:rv(?:ice|er)|curity|quence|ssion|arch)|y(?:mmetric\\s*key|nonym)|togroup|chema)|m(?:a(?:s(?:ter\\s*key|k)|terialized)|e(?:ssage\\s*type|thod)|odule)|l(?:o(?:g(?:file\\s*group|in)|ckdown)|a(?:ngua|r)ge|ibrary)|t(?:(?:abl(?:espac)?|yp)e|r(?:igger|usted)|hreshold|ext)|p(?:a(?:rtition|ckage)|ro(?:cedur|fil)e|ermission)|d(?:i(?:mension|skgroup)|atabase|efault|omain)|r(?:o(?:l(?:lback|e)|ute)|e(?:sourc|mot)e)|f(?:u(?:lltext|nction)|lashback|oreign)|e(?:xte(?:nsion|rnal)|(?:ndpoi|ve)nt)|in(?:dex(?:type)?|memory|stance)|b(?:roker\\s*priority|ufferpool)|x(?:ml\\s*schema|srobject)|w(?:ork(?:load)?|rapper)|hi(?:erarchy|stogram)|o(?:perator|utline)|(?:nicknam|queu)e|us(?:age|er)|group|java|view)|u(?:nion\\s*(?:(?:distin|sele)ct|all)|pdate)|d(?:e(?:lete|sc)|rop)|(?:truncat|renam)e|(?:inser|selec)t|load)\\b|create\\s+\\w+)|(?:(?:(?:trunc|cre|upd)at|renam)e|(?:inser|selec)t|de(?:lete|sc)|alter|load)\\s+(?:group_concat|load_file|char)\\s?\\(?|[\\d\\W]\\s+as\\b\\s*[\"'`\\w]+\\s*\\bfrom|[\\s(]load_file\\s*?\\(|[\"'`]\\s+regexp\\W|end\\s*?\\);)",
       "(?i)(?:[\"'`](?:\\s*?(?:(?:\\*.+(?:(?:an|i)d|between|like|x?or|div)\\W*?[\"'`]|(?:between|like|x?or|and|div)\\s[^\\d]+[\\w-]+.*?)\\d|[^\\w\\s?]+\\s*?[^\\w\\s]+\\s*?[\"'`]|[^\\w\\s]+\\s*?[\\W\\d].*?(?:--|#))|.*?\\*\\s*?\\d)|[()\\*<>%+-][\\w-]+[^\\w\\s]+[\"'`][^,]|\\^[\"'`])",
       "(?i)(?:\\b(?:having\\b(?: ?(?:[\\'\"][^=]{1,10}[\\'\" ?[=<>]+|\\d{1,10} ?[=<>]+)|\\s+(?:'[^=]{1,10}'|\\d{1,10})\\s*?[=<>])|ex(?:ecute(?:\\s{1,5}[\\w\\.$]{1,5}\\s{0,3}|\\()|ists\\s*?\\(\\s*?select\\b)|(?:create\\s+?table.{0,20}?|like\\W*?char\\W*?)\\()|exists\\s(?:s(?:elect\\S(?:if(?:null)?\\s\\(|concat|top)|ystem\\s\\()|\\bhaving\\b\\s+\\d{1,10}|'[^=]{1,10}'|\\sselect)|select.*?case|from.*?limit|order\\sby)",
       "(?i)(?:\\b(?:or\\b(?:\\s+(?:'[^=]{1,10}'(?:\\s*?[=<>])?|\\d{1,10}(?:\\s*?[=<>])?)|\\s?(?:[\\'\"][^=]{1,10}[\\'\"]|\\d{1,10})\\s?[=<>]+)|xor\\b\\s+(?:'[^=]{1,10}'(?:\\s*?[=<>])?|\\d{1,10}(?:\\s*?[=<>])?))|'\\s+x?or\\s+.{1,20}[+\\-!<>=])",
       "(?i)\\band\\b(?: ?(?:[\\'\"][^=]{1,10}[\\'\"]|\\d{1,10}) ?[=<>]+|\\s+(?:\\d{1,10}\\s*?[=<>]|'[^=]{1,10}'))",
       "(?i)\\b(?:c(?:o(?:n(?:v(?:ert(?:_tz)?)?|cat(?:_ws)?|nection_id)|(?:mpres)?s|ercibility|(?:un)?t|alesce)|ur(?:rent_(?:time(?:stamp)?|date|user)|(?:dat|tim)e)|h(?:ar(?:(?:acter)?_length|set)?|r)|iel(?:ing)?|ast|r32)|s(?:t(?:d(?:dev(?:_(?:sam|po)p)?)?|r(?:_to_date|cmp))|u(?:b(?:str(?:ing(?:_index)?)?|(?:dat|tim)e)|m)|e(?:c(?:_to_time|ond)|ssion_user)|ys(?:tem_user|date)|ha[12]?|oundex|chema|ig?n|leep|pace|qrt)|i(?:s(?:_(?:ipv(?:4(?:_(?:compat|mapped))?|6)|n(?:ot(?:_null)?|ull)|(?:free|used)_lock)|null)?|n(?:et(?:6_(?:aton|ntoa)|_(?:aton|ntoa))|s(?:ert|tr)|terval)?|f(?:null)?)|d(?:a(?:t(?:e(?:_(?:format|add|sub)|diff)?|abase)|y(?:of(?:month|week|year)|name)?)|e(?:(?:s_(?:de|en)cryp|faul)t|grees|code)|count|ump)|l(?:o(?:ca(?:l(?:timestamp)?|te)|g(?:10|2)?|ad_file|wer)|ast(?:_(?:insert_id|day))?|e(?:(?:as|f)t|ngth)|case|trim|pad|n)|u(?:n(?:compress(?:ed_length)?|ix_timestamp|hex)|tc_(?:time(?:stamp)?|date)|p(?:datexml|per)|uid(?:_short)?|case|ser)|r(?:a(?:wto(?:nhex(?:toraw)?|hex)|dians|nd)|e(?:p(?:lace|eat)|lease_lock|verse)|o(?:w_count|und)|ight|trim|pad)|t(?:ime(?:_(?:format|to_sec)|stamp(?:diff|add)?|diff)?|o_(?:(?:second|day)s|base64|n?char)|r(?:uncate|im)|an)|m(?:a(?:ke(?:_set|date)|ster_pos_wait|x)|i(?:(?:crosecon)?d|n(?:ute)?)|o(?:nth(?:name)?|d)|d5)|f(?:i(?:eld(?:_in_set)?|nd_in_set)|rom_(?:unixtime|base64|days)|o(?:und_rows|rmat)|loor)|p(?:o(?:w(?:er)?|sition)|eriod_(?:diff|add)|rocedure_analyse|assword|g_sleep|i)|a(?:s(?:cii(?:str)?|in)|es_(?:de|en)crypt|dd(?:dat|tim)e|(?:co|b)s|tan2?|vg)|b(?:i(?:t_(?:length|count|x?or|and)|n(?:_to_num)?)|enchmark)|e(?:x(?:tract(?:value)?|p(?:ort_set)?)|nc(?:rypt|ode)|lt)|g(?:r(?:oup_conca|eates)t|et_(?:format|lock))|v(?:a(?:r(?:_(?:sam|po)p|iance)|lues)|ersion)|o(?:(?:ld_passwo)?rd|ct(?:et_length)?)|we(?:ek(?:ofyear|day)?|ight_string)|n(?:o(?:t_in|w)|ame_const|ullif)|h(?:ex(?:toraw)?|our)|qu(?:arter|ote)|year(?:week)?|xmltype)\\W*?\\(",
       "(?i)(?:xp_(?:reg(?:re(?:movemultistring|ad)|delete(?:value|key)|enum(?:value|key)s|addmultistring|write)|(?:servicecontro|cmdshel)l|e(?:xecresultset|numdsn)|ntsec(?:_enumdomains)?|terminate(?:_process)?|availablemedia|loginconfig|filelist|dirtree|makecab)|s(?:p_(?:(?:addextendedpro|sqlexe)c|p(?:assword|repare)|replwritetovarbin|is_srvrolemember|execute(?:sql)?|makewebtask|oacreate|help)|ql_(?:longvarchar|variant))|open(?:owa_util|rowset|query)|(?:n?varcha|tbcreato)r|autonomous_transaction|db(?:a_users|ms_java)|utl_(?:file|http))",
       "(?i)(?:\\b(?:(?:s(?:elect\\b.{1,100}?\\b(?:(?:(?:length|count)\\b.{1,100}?|.*?\\bdump\\b.*)\\bfrom|to(?:p\\b.{1,100}?\\bfrom|_(?:numbe|cha)r)|(?:from\\b.{1,100}?\\bwher|data_typ)e|instr)|ys_context)|in(?:to\\b\\W*?\\b(?:dump|out)file|sert\\b\\W*?\\binto|ner\\b\\W*?\\bjoin)|u(?:nion\\b.{1,100}?\\bselect|tl_inaddr)|group\\b.*?\\bby\\b.{1,100}?\\bhaving|d(?:elete\\b\\W*?\\bfrom|bms_\\w+\\.)|load\\b\\W*?\\bdata\\b.*?\\binfile)\\b|print\\b\\W*?\\@\\@)|(?:;\\W*?\\b(?:shutdown|drop)|collation\\W*?\\(a|\\@\\@version)\\b|'(?:s(?:qloledb|a)|msdasql|dbo)')",
       "(?i:\\b0x[a-f\\d]{3,})",
       "(?:`(?:(?:[\\w\\s=_\\-+{}()<@]){2,29}|(?:[A-Za-z0-9+/]{4})+(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=)?)`)",
       "(?i)\\W+\\d*?\\s*?\\bhaving\\b\\s*?[^\\s\\-]",
       "[\"'`][\\s\\d]*?[^\\w\\s]\\W*?\\d\\W*?.*?[\"'`\\d]",
       "((?:[~!@#\\$%\\^&\\*\\(\\)\\-\\+=\\{\\}\\[\\]\\|:;\"'´’‘`<>][^~!@#\\$%\\^&\\*\\(\\)\\-\\+=\\{\\}\\[\\]\\|:;\"'´’‘`<>]*?){8})",
       "(?:'(?:(?:[\\w\\s=_\\-+{}()<@]){2,29}|(?:[A-Za-z0-9+/]{4})+(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=)?)')",
       "((?:[~!@#\\$%\\^&\\*\\(\\)\\-\\+=\\{\\}\\[\\]\\|:;\"'´’‘`<>][^~!@#\\$%\\^&\\*\\(\\)\\-\\+=\\{\\}\\[\\]\\|:;\"'´’‘`<>]*?){3})",
       "(?i:\\.cookie\\b.*?;\\W*?(?:expires|domain)\\W*?=|\\bhttp-equiv\\W+set-cookie\\b)",
       "java\\.lang\\.(?:runtime|processbuilder)",
       "(?i)(?:\\$|&dollar;?)(?:\\{|&(?:lbrace|lcub);?)(?:[^}]{0,15}(?:\\$|&dollar;?)(?:\\{|&(?:lbrace|lcub);?)|(?:jndi|ctx))",
       "(?i)(?:\\$|&dollar;?)(?:\\{|&(?:lbrace|lcub);?)(?:[^}]*(?:\\$|&dollar;?)(?:\\{|&(?:lbrace|lcub);?)|(?:jndi|ctx))",
       "\\xac\\xed\\x00\\x05",
       "(?:rO0ABQ|KztAAU|Cs7QAF)",
       "(?:clonetransformer|forclosure|instantiatefactory|instantiatetransformer|invokertransformer|prototypeclonefactory|prototypeserializationfactory|whileclosure|getproperty|filewriter|xmldecoder)",
       "java\\b.+(?:runtime|processbuilder)",
       "(?:class\\.module\\.classLoader\\.resources\\.context\\.parent\\.pipeline|springframework\\.context\\.support\\.FileSystemXmlApplicationContext)",
       "(?:cnVudGltZQ|HJ1bnRpbWU|BydW50aW1l|cHJvY2Vzc2J1aWxkZXI|HByb2Nlc3NidWlsZGVy|Bwcm9jZXNzYnVpbGRlcg|Y2xvbmV0cmFuc2Zvcm1lcg|GNsb25ldHJhbnNmb3JtZXI|BjbG9uZXRyYW5zZm9ybWVy|Zm9yY2xvc3VyZQ|GZvcmNsb3N1cmU|Bmb3JjbG9zdXJl|aW5zdGFudGlhdGVmYWN0b3J5|Gluc3RhbnRpYXRlZmFjdG9yeQ|BpbnN0YW50aWF0ZWZhY3Rvcnk|aW5zdGFudGlhdGV0cmFuc2Zvcm1lcg|Gluc3RhbnRpYXRldHJhbnNmb3JtZXI|BpbnN0YW50aWF0ZXRyYW5zZm9ybWVy|aW52b2tlcnRyYW5zZm9ybWVy|Gludm9rZXJ0cmFuc2Zvcm1lcg|BpbnZva2VydHJhbnNmb3JtZXI|cHJvdG90eXBlY2xvbmVmYWN0b3J5|HByb3RvdHlwZWNsb25lZmFjdG9yeQ|Bwcm90b3R5cGVjbG9uZWZhY3Rvcnk|cHJvdG90eXBlc2VyaWFsaXphdGlvbmZhY3Rvcnk|HByb3RvdHlwZXNlcmlhbGl6YXRpb25mYWN0b3J5|Bwcm90b3R5cGVzZXJpYWxpemF0aW9uZmFjdG9yeQ|d2hpbGVjbG9zdXJl|HdoaWxlY2xvc3VyZQ|B3aGlsZWNsb3N1cmU)",
       "(?i)(?:\\$|&dollar;?)(?:\\{|&(?:lbrace|lcub);?)"});
  // clang-format on
}

// NOLINTNEXTLINE(readability-identifier-naming)
static void BM_RE2Single(benchmark::State& state) {
  const std::string input = clusterInput(state.range(0));
  std::vector<std::shared_ptr<re2::RE2>> res;
  for (auto& pattern : clusterRePatterns()) {
    res.emplace_back(std::make_shared<re2::RE2>(pattern.c_str()));
  }
  uint32_t passes = 0;
  for (auto _ : state) { // NOLINT
    for (auto& re : res) {
      if (re2::RE2::FullMatch(input, *re)) {
        ++passes;
      }
    }
  }
  RELEASE_ASSERT(passes == 0, "");
}
BENCHMARK(BM_RE2Single)->RangeMultiplier(10)->Range(10, 1000);

// NOLINTNEXTLINE(readability-identifier-naming)
static void BM_RE2Set(benchmark::State& state) {
  const std::string input = clusterInput(state.range(0));
  re2::RE2::Set re(RE2::DefaultOptions, RE2::UNANCHORED);
  for (auto& pattern : clusterRePatterns()) {
    RELEASE_ASSERT(re.Add(pattern, nullptr) >= 0, "");
  }
  RELEASE_ASSERT(re.Compile(), "");
  uint32_t passes = 0;
  for (auto _ : state) { // NOLINT
    if (re.Match(input, nullptr)) {
      ++passes;
    }
  }
  RELEASE_ASSERT(passes == 0, "");
}
BENCHMARK(BM_RE2Set)->RangeMultiplier(10)->Range(10, 1000);

// NOLINTNEXTLINE(readability-identifier-naming)
static void BM_HyperscanSingle(benchmark::State& state) {
  const std::string input = clusterInput(state.range(0));
  std::vector<hs_database_t*> databases(clusterRePatterns().size(), {});
  std::vector<hs_scratch_t*> scratches(clusterRePatterns().size(), {});
  hs_compile_error_t* compile_err;
  for (size_t i = 0; i < clusterRePatterns().size(); i++) {
    RELEASE_ASSERT(hs_compile_multi(std::vector<const char*>{clusterRePatterns()[i].c_str()}.data(),
                                    nullptr, nullptr, 1, HS_MODE_BLOCK, nullptr, &databases[i],
                                    &compile_err) == HS_SUCCESS,
                   "");
    RELEASE_ASSERT(hs_alloc_scratch(databases[i], &scratches[i]) == HS_SUCCESS, "");
  }
  uint32_t passes = 0;
  for (auto _ : state) { // NOLINT
    for (size_t i = 0; i < clusterRePatterns().size(); i++) {
      hs_error_t err = hs_scan(
          databases[i], input.data(), input.size(), 0, scratches[i],
          [](unsigned int, unsigned long long, unsigned long long, unsigned int,
             void* context) -> int {
            uint32_t* passes = static_cast<uint32_t*>(context);
            *passes = *passes + 1;

            return 1;
          },
          &passes);
      ASSERT(err == HS_SUCCESS || err == HS_SCAN_TERMINATED);
    }
  }
  for (size_t i = 0; i < clusterRePatterns().size(); i++) {
    hs_free_scratch(scratches[i]);
    hs_free_database(databases[i]);
  }
  RELEASE_ASSERT(passes == 0, "");
}
BENCHMARK(BM_HyperscanSingle)->RangeMultiplier(10)->Range(10, 1000);

// NOLINTNEXTLINE(readability-identifier-naming)
static void BM_HyperscanMulti(benchmark::State& state) {
  const std::string input = clusterInput(state.range(0));
  hs_database_t* database{};
  hs_scratch_t* scratch{};
  hs_compile_error_t* compile_err;
  std::vector<const char*> patterns;
  for (auto& pattern : clusterRePatterns()) {
    patterns.push_back(pattern.c_str());
  }
  RELEASE_ASSERT(hs_compile_multi(patterns.data(), nullptr, nullptr, patterns.size(), HS_MODE_BLOCK,
                                  nullptr, &database, &compile_err) == HS_SUCCESS,
                 "");
  RELEASE_ASSERT(hs_alloc_scratch(database, &scratch) == HS_SUCCESS, "");
  uint32_t passes = 0;
  for (auto _ : state) { // NOLINT
    hs_error_t err = hs_scan(
        database, input.data(), input.size(), 0, scratch,
        [](unsigned int, unsigned long long, unsigned long long, unsigned int,
           void* context) -> int {
          uint32_t* passes = static_cast<uint32_t*>(context);
          *passes = *passes + 1;

          return 1;
        },
        &passes);
    ASSERT(err == HS_SUCCESS || err == HS_SCAN_TERMINATED);
  }
  hs_free_scratch(scratch);
  hs_free_database(database);
  RELEASE_ASSERT(passes == 0, "");
}
BENCHMARK(BM_HyperscanMulti)->RangeMultiplier(10)->Range(10, 1000);

// NOLINTNEXTLINE(readability-identifier-naming)
static void BM_RE2(benchmark::State& state) {
  const std::string input = clusterInput(state.range(0));
  re2::RE2 re(clusterRePatterns()[state.range(1)].c_str());
  for (auto _ : state) { // NOLINT
    re2::RE2::FullMatch(input, re);
  }
}
BENCHMARK(BM_RE2)->ArgsProduct({benchmark::CreateRange(10, 1000, 10),
                                benchmark::CreateDenseRange(0, clusterRePatterns().size() - 1, 1)});

// NOLINTNEXTLINE(readability-identifier-naming)
static void BM_Hyperscan(benchmark::State& state) {
  const std::string input = clusterInput(state.range(0));
  hs_database_t* database{};
  hs_scratch_t* scratch{};
  hs_compile_error_t* compile_err;
  RELEASE_ASSERT(
      hs_compile_multi(std::vector<const char*>{clusterRePatterns()[state.range(1)].c_str()}.data(),
                       nullptr, nullptr, 1, HS_MODE_BLOCK, nullptr, &database,
                       &compile_err) == HS_SUCCESS,
      "");
  RELEASE_ASSERT(hs_alloc_scratch(database, &scratch) == HS_SUCCESS, "");
  for (auto _ : state) { // NOLINT
    hs_error_t err = hs_scan(
        database, input.data(), input.size(), 0, scratch,
        [](unsigned int, unsigned long long, unsigned long long, unsigned int, void*) -> int {
          return 1;
        },
        nullptr);
    ASSERT(err == HS_SUCCESS || err == HS_SCAN_TERMINATED);
  }
  hs_free_scratch(scratch);
  hs_free_database(database);
}
BENCHMARK(BM_Hyperscan)
    ->ArgsProduct({benchmark::CreateRange(10, 1000, 10),
                   benchmark::CreateDenseRange(0, clusterRePatterns().size() - 1, 1)});

// NOLINTNEXTLINE(readability-identifier-naming)
static void BM_CompiledGoogleReMatcherSingle(benchmark::State& state) {
  const std::string input = clusterInput(state.range(0));
  std::vector<std::shared_ptr<Regex::CompiledGoogleReMatcher>> matchers;
  for (auto& pattern : clusterRePatterns()) {
    matchers.emplace_back(std::make_shared<Regex::CompiledGoogleReMatcher>(pattern, false));
  }
  uint32_t passes = 0;
  for (auto _ : state) { // NOLINT
    for (auto& matcher : matchers) {
      if (matcher->match(input)) {
        ++passes;
      }
    }
  }
  RELEASE_ASSERT(passes == 0, "");
}
BENCHMARK(BM_CompiledGoogleReMatcherSingle)->RangeMultiplier(10)->Range(10, 1000);

// NOLINTNEXTLINE(readability-identifier-naming)
static void BM_HyperscanMatcherSingle(benchmark::State& state) {
  const std::string input = clusterInput(state.range(0));
  auto instance = ThreadLocal::InstanceImpl();
  std::vector<std::shared_ptr<Extensions::Matching::InputMatchers::Hyperscan::Matcher>> matchers;
  for (auto& pattern : clusterRePatterns()) {
    matchers.emplace_back(std::make_shared<Extensions::Matching::InputMatchers::Hyperscan::Matcher>(
        std::vector<const char*>{pattern.c_str()}, std::vector<unsigned int>{0},
        std::vector<unsigned int>{0}, instance, false));
  }
  uint32_t passes = 0;
  for (auto _ : state) { // NOLINT
    for (auto& matcher : matchers) {
      if (matcher->match(input)) {
        ++passes;
      }
    }
  }
  RELEASE_ASSERT(passes == 0, "");
}
BENCHMARK(BM_HyperscanMatcherSingle)->RangeMultiplier(10)->Range(10, 1000);

// NOLINTNEXTLINE(readability-identifier-naming)
static void BM_HyperscanMatcherMulti(benchmark::State& state) {
  const std::string input = clusterInput(state.range(0));
  auto instance = ThreadLocal::InstanceImpl();
  std::vector<const char*> patterns;
  for (auto& pattern : clusterRePatterns()) {
    patterns.push_back(pattern.c_str());
  }
  std::vector<unsigned int> zeros(patterns.size(), 0);
  auto matcher = Extensions::Matching::InputMatchers::Hyperscan::Matcher(patterns, zeros, zeros,
                                                                         instance, false);
  uint32_t passes = 0;
  for (auto _ : state) { // NOLINT
    if (matcher.match(input)) {
      ++passes;
    }
  }
  RELEASE_ASSERT(passes == 0, "");
}
BENCHMARK(BM_HyperscanMatcherMulti)->RangeMultiplier(10)->Range(10, 1000);

// NOLINTNEXTLINE(readability-identifier-naming)
static void BM_CompiledGoogleReMatcher(benchmark::State& state) {
  const std::string input = clusterInput(state.range(0));
  const auto matcher = Regex::CompiledGoogleReMatcher(clusterRePatterns()[state.range(1)], false);
  for (auto _ : state) { // NOLINT
    matcher.match(input);
  }
}
BENCHMARK(BM_CompiledGoogleReMatcher)
    ->ArgsProduct({benchmark::CreateRange(10, 1000, 10),
                   benchmark::CreateDenseRange(0, clusterRePatterns().size() - 1, 1)});

// NOLINTNEXTLINE(readability-identifier-naming)
static void BM_HyperscanMatcher(benchmark::State& state) {
  const std::string input = clusterInput(state.range(0));
  auto instance = ThreadLocal::InstanceImpl();
  auto matcher = Extensions::Matching::InputMatchers::Hyperscan::Matcher(
      {clusterRePatterns()[state.range(1)].c_str()}, {0}, {0}, instance, false);
  for (auto _ : state) { // NOLINT
    matcher.match(input);
  }
}
BENCHMARK(BM_HyperscanMatcher)
    ->ArgsProduct({benchmark::CreateRange(10, 1000, 10),
                   benchmark::CreateDenseRange(0, clusterRePatterns().size() - 1, 1)});

} // namespace Envoy
