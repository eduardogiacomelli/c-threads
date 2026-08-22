/**
 * A small C tokenizer.
 *
 * Hand-written rather than pulled in from Shiki/Prism because the point is
 * not colour: identifiers have to stay addressable so the code pane can bind
 * the `p` in the source to the live slot named `p` and show its value on
 * hover. A generic highlighter hands back opaque markup.
 *
 * Block comments are tracked across lines with a real state flag - the
 * "does this line start with a star" heuristic misreads `*p = 30;`.
 */

export type TokenKind =
  | "keyword"
  | "type"
  | "ident"
  | "number"
  | "string"
  | "char"
  | "comment"
  | "preproc"
  | "punct"
  | "op"
  | "fn"
  | "space";

export interface Token {
  kind: TokenKind;
  text: string;
}

const KEYWORDS = new Set([
  "if", "else", "for", "while", "do", "return", "break", "continue",
  "switch", "case", "default", "goto", "sizeof", "static", "const",
  "struct", "typedef", "enum", "union", "extern", "volatile", "register",
]);

const TYPES = new Set([
  "int", "char", "void", "long", "short", "float", "double", "unsigned",
  "signed", "size_t", "pthread_t", "NULL",
]);

const PUNCT = new Set(["(", ")", "{", "}", "[", "]", ";", ",", "."]);

const OPS2 = [
  "->", "++", "--", "==", "!=", "<=", ">=", "&&", "||",
  "+=", "-=", "*=", "/=", "%=", "<<", ">>",
];

/** Tokenizes a whole file, returning one token list per line. */
export function tokenizeSource(source: string): Token[][] {
  let inBlock = false;
  return source.split("\n").map((line) => {
    const { tokens, stillOpen } = tokenizeLine(line, inBlock);
    inBlock = stillOpen;
    return tokens;
  });
}

function tokenizeLine(line: string, startsInBlock: boolean) {
  const out: Token[] = [];
  let i = 0;
  let inBlock = startsInBlock;

  if (inBlock) {
    const end = line.indexOf("*/");
    if (end < 0) {
      return { tokens: [{ kind: "comment" as const, text: line }], stillOpen: true };
    }
    out.push({ kind: "comment", text: line.slice(0, end + 2) });
    i = end + 2;
    inBlock = false;
  }

  let sawCode = false;

  while (i < line.length) {
    const c = line[i];

    if (/\s/.test(c)) {
      let j = i;
      while (j < line.length && /\s/.test(line[j])) j++;
      out.push({ kind: "space", text: line.slice(i, j) });
      i = j;
      continue;
    }

    if (c === "#" && !sawCode) {
      out.push({ kind: "preproc", text: line.slice(i) });
      break;
    }

    if (c === "/" && line[i + 1] === "/") {
      out.push({ kind: "comment", text: line.slice(i) });
      break;
    }

    if (c === "/" && line[i + 1] === "*") {
      const end = line.indexOf("*/", i + 2);
      if (end < 0) {
        out.push({ kind: "comment", text: line.slice(i) });
        inBlock = true;
        break;
      }
      out.push({ kind: "comment", text: line.slice(i, end + 2) });
      i = end + 2;
      continue;
    }

    sawCode = true;

    if (c === '"' || c === "'") {
      let j = i + 1;
      while (j < line.length && line[j] !== c) {
        if (line[j] === "\\") j++;
        j++;
      }
      out.push({
        kind: c === '"' ? "string" : "char",
        text: line.slice(i, Math.min(j + 1, line.length)),
      });
      i = j + 1;
      continue;
    }

    if (/[0-9]/.test(c)) {
      let j = i;
      while (j < line.length && /[0-9a-fA-FxX.]/.test(line[j])) j++;
      out.push({ kind: "number", text: line.slice(i, j) });
      i = j;
      continue;
    }

    if (/[A-Za-z_]/.test(c)) {
      let j = i;
      while (j < line.length && /[A-Za-z0-9_]/.test(line[j])) j++;
      const text = line.slice(i, j);
      let kind: TokenKind = "ident";
      if (KEYWORDS.has(text)) kind = "keyword";
      else if (TYPES.has(text)) kind = "type";
      else if (line[j] === "(") kind = "fn";
      out.push({ kind, text });
      i = j;
      continue;
    }

    if (PUNCT.has(c)) {
      out.push({ kind: "punct", text: c });
      i++;
      continue;
    }

    const two = line.slice(i, i + 2);
    if (OPS2.includes(two)) {
      out.push({ kind: "op", text: two });
      i += 2;
      continue;
    }

    out.push({ kind: "op", text: c });
    i++;
  }

  return { tokens: out, stillOpen: inBlock };
}

export const TOKEN_CLASS: Record<TokenKind, string> = {
  keyword: "text-[var(--syn-keyword)]",
  type: "text-[var(--syn-type)]",
  ident: "text-[var(--syn-ident)]",
  number: "text-[var(--syn-number)]",
  string: "text-[var(--syn-string)]",
  char: "text-[var(--syn-string)]",
  comment: "text-[var(--syn-comment)] italic",
  preproc: "text-[var(--syn-preproc)]",
  punct: "text-[var(--syn-punct)]",
  op: "text-[var(--syn-op)]",
  fn: "text-[var(--syn-fn)]",
  space: "",
};
