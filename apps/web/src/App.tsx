import { Children, isValidElement, useCallback, type ReactNode, useEffect, useMemo, useRef, useState } from "react";
import x86asm from "highlight.js/lib/languages/x86asm";
import { common } from "lowlight";
import ReactMarkdown from "react-markdown";
import rehypeHighlight from "rehype-highlight";
import remarkBreaks from "remark-breaks";
import remarkGfm from "remark-gfm";
import { books } from "./books";
import { MermaidBlock } from "./components/MermaidBlock";
import { QuizCard } from "./components/QuizCard";
import { RunnableAsmBlock } from "./components/RunnableAsmBlock";
import { TopicBadge } from "./components/TopicBadge";

type Theme = "light" | "dark";

const rehypeHighlightOptions = {
  languages: {
    ...common,
    x86asm,
  },
  aliases: {
    x86asm: ["asm", "nasm"],
  },
} as const;

function initialTheme(): Theme {
  if (typeof window === "undefined") {
    return "light";
  }

  const stored = window.localStorage.getItem("cs101-theme");
  if (stored === "light" || stored === "dark") {
    return stored;
  }

  return window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
}

function extractText(node: ReactNode): string {
  if (typeof node === "string" || typeof node === "number") {
    return String(node);
  }

  if (Array.isArray(node)) {
    return node.map(extractText).join("");
  }

  if (isValidElement<{ children?: ReactNode }>(node)) {
    return extractText(node.props.children);
  }

  return Children.toArray(node).map(extractText).join("");
}

/** 見出しテキストから URL-safe なスラグを生成する */
function slugify(text: string): string {
  return text
    .trim()
    .replace(/\s+/g, "-")
    .replace(/[#]/g, "")
    .toLowerCase();
}

/** ハッシュ文字列を { stageId, sectionSlug? } にパースする。
 *  形式: "#v6" or "#v6/セクション名" */
function parseHash(raw: string): { stageId: string; sectionSlug?: string } {
  const hash = raw.replace(/^#/, "");
  const slashIdx = hash.indexOf("/");
  if (slashIdx === -1) {
    return { stageId: hash };
  }
  return {
    stageId: hash.slice(0, slashIdx),
    sectionSlug: decodeURIComponent(hash.slice(slashIdx + 1)),
  };
}

function initialStageId(): string {
  if (typeof window === "undefined") {
    return books[0].id;
  }

  const { stageId } = parseHash(window.location.hash);
  if (stageId && books.some((entry) => entry.id === stageId)) {
    return stageId;
  }

  return books[0].id;
}

function navigateTo(id: string, setActiveId: (id: string) => void) {
  setActiveId(id);
  history.pushState(null, "", `#${id}`);
}

function headingWithId(Tag: "h1" | "h2" | "h3" | "h4") {
  return function Heading({ children }: { children?: ReactNode }) {
    const text = extractText(children as ReactNode);
    return <Tag id={slugify(text)}>{children}</Tag>;
  };
}

function App() {
  const [activeId, setActiveId] = useState(initialStageId);
  const [theme, setTheme] = useState<Theme>(initialTheme);
  const pendingScrollRef = useRef<string | null>(null);
  const activeBook = useMemo(
    () => books.find((entry) => entry.id === activeId) ?? books[0],
    [activeId]
  );

  /** セクションへスクロール（DOM 更新後に呼ぶ） */
  const scrollToSection = useCallback((slug: string) => {
    const el = document.getElementById(slug);
    if (el) {
      el.scrollIntoView({ behavior: "smooth", block: "start" });
      pendingScrollRef.current = null;
    }
  }, []);

  /** ステージ + セクションへ遷移 */
  const navigateToSection = useCallback(
    (stageId: string, sectionSlug?: string) => {
      const hashStr = sectionSlug ? `${stageId}/${sectionSlug}` : stageId;
      if (stageId !== activeId) {
        pendingScrollRef.current = sectionSlug ?? null;
        setActiveId(stageId);
        history.pushState(null, "", `#${hashStr}`);
      } else if (sectionSlug) {
        history.pushState(null, "", `#${hashStr}`);
        scrollToSection(sectionSlug);
      }
    },
    [activeId, scrollToSection]
  );

  /** ページ切り替え後の pending スクロールを実行 */
  useEffect(() => {
    if (pendingScrollRef.current) {
      requestAnimationFrame(() => {
        if (pendingScrollRef.current) {
          scrollToSection(pendingScrollRef.current);
        }
      });
    }
  }, [activeId, scrollToSection]);

  /** 初回ロード時にセクション指定があればスクロール */
  useEffect(() => {
    const { sectionSlug } = parseHash(window.location.hash);
    if (sectionSlug) {
      requestAnimationFrame(() => scrollToSection(sectionSlug));
    }
  }, []); // eslint-disable-line react-hooks/exhaustive-deps

  useEffect(() => {
    document.documentElement.dataset.theme = theme;
    document.documentElement.style.colorScheme = theme;
    window.localStorage.setItem("cs101-theme", theme);
  }, [theme]);

  useEffect(() => {
    function handlePopState() {
      const { stageId, sectionSlug } = parseHash(window.location.hash);
      if (stageId && books.some((entry) => entry.id === stageId)) {
        if (stageId !== activeId) {
          pendingScrollRef.current = sectionSlug ?? null;
          setActiveId(stageId);
        } else if (sectionSlug) {
          scrollToSection(sectionSlug);
        }
      }
    }

    window.addEventListener("popstate", handlePopState);
    return () => window.removeEventListener("popstate", handlePopState);
  }, [activeId, scrollToSection]);

  return (
    <div className="reader-shell">
      <aside className="reader-sidebar">
        <div className="sidebar-copy">
          <p>CPU の基本動作を、理論・図・実装・テストを往復しながら読むためのノートです。</p>
        </div>

        <nav className="stage-nav" aria-label="book stages">
          {books.map((book) => (
            <button
              key={book.id}
              className={book.id === activeBook.id ? "stage-link active" : "stage-link"}
              onClick={() => navigateTo(book.id, setActiveId)}
              type="button"
            >
              <span className="stage-meta">
                <TopicBadge topic={book.topic} />
                <span className="stage-id">{book.id}</span>
              </span>
              <strong>{book.title}</strong>
            </button>
          ))}
        </nav>
      </aside>

      <main className="reader-main">
        <header className="reader-header">
          <div className="header-copy">
            <div className="header-stage">
              <TopicBadge topic={activeBook.topic} />
              <p className="stage-pill">{activeBook.id}</p>
            </div>
            <div>
              <h2>{activeBook.title}</h2>
            </div>
          </div>
          <button
            className="theme-toggle"
            onClick={() => setTheme((current) => (current === "light" ? "dark" : "light"))}
            type="button"
          >
            {theme === "light" ? "Dark" : "Light"}
          </button>
        </header>

        <article className="markdown-body">
          <ReactMarkdown
            remarkPlugins={[remarkGfm, remarkBreaks]}
            rehypePlugins={[[rehypeHighlight, rehypeHighlightOptions]]}
            components={{
              h1: headingWithId("h1"),
              h2: headingWithId("h2"),
              h3: headingWithId("h3"),
              h4: headingWithId("h4"),
              a({ href, children }) {
                if (href?.startsWith("#")) {
                  const { stageId, sectionSlug } = parseHash(href);
                  if (stageId && books.some((entry) => entry.id === stageId)) {
                    return (
                      <a
                        href={href}
                        onClick={(e) => {
                          e.preventDefault();
                          navigateToSection(stageId, sectionSlug);
                        }}
                      >
                        {children}
                      </a>
                    );
                  }
                }
                return <a href={href}>{children}</a>;
              },
              blockquote({ children }) {
                const text = extractText(children as ReactNode);
                const match = text.match(/^Column:\s*(.+?)(?:\n|$)/);
                if (match) {
                  const title = match[1];
                  const items = Children.toArray(children as ReactNode);
                  const body = items.map((child, i) => {
                    if (i === 0 && isValidElement<{ children?: ReactNode }>(child)) {
                      const inner = Children.toArray(child.props.children);
                      const filtered = inner.filter((c) => {
                        if (!isValidElement<{ children?: ReactNode }>(c)) return true;
                        const t = extractText(c.props.children);
                        return !t.startsWith("Column:");
                      });
                      if (filtered.length === 0) return null;
                      return <p key={i}>{filtered}</p>;
                    }
                    return child;
                  });
                  return (
                    <aside className="column-box">
                      <strong className="column-box__title">{title}</strong>
                      {body}
                    </aside>
                  );
                }
                return <blockquote>{children}</blockquote>;
              },
              pre({ children }) {
                if (
                  isValidElement<{ className?: string; children?: unknown }>(children) &&
                  typeof children.props.className === "string" &&
                  children.props.className.includes("language-mermaid")
                ) {
                  const chart = String(children.props.children).replace(/\n$/, "");
                  return <MermaidBlock chart={chart} theme={theme} />;
                }

                if (
                  isValidElement<{ className?: string; children?: unknown }>(children) &&
                  typeof children.props.className === "string" &&
                  children.props.className.includes("language-asm") &&
                  /^v[1-8]$/.test(activeBook.id)
                ) {
                  const source = extractText(children.props.children as ReactNode).replace(/\n$/, "");
                  return <RunnableAsmBlock initialSource={source} stageId={activeBook.id} />;
                }

                return <pre>{children}</pre>;
              },
            }}
          >
            {activeBook.markdown}
          </ReactMarkdown>

          {activeBook.quiz ? (
            <QuizCard key={activeBook.id} stageId={activeBook.id} quiz={activeBook.quiz} />
          ) : null}
        </article>
      </main>
    </div>
  );
}

export default App;
