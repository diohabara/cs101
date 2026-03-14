import { isValidElement, useEffect, useMemo, useState } from "react";
import ReactMarkdown from "react-markdown";
import rehypeHighlight from "rehype-highlight";
import remarkBreaks from "remark-breaks";
import remarkGfm from "remark-gfm";
import { books } from "./books";
import { MermaidBlock } from "./components/MermaidBlock";
import { QuizCard } from "./components/QuizCard";
import { TopicBadge } from "./components/TopicBadge";

type Theme = "light" | "dark";

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

function App() {
  const [activeId, setActiveId] = useState(books[0].id);
  const [theme, setTheme] = useState<Theme>(initialTheme);
  const activeBook = useMemo(
    () => books.find((entry) => entry.id === activeId) ?? books[0],
    [activeId]
  );

  useEffect(() => {
    document.documentElement.dataset.theme = theme;
    document.documentElement.style.colorScheme = theme;
    window.localStorage.setItem("cs101-theme", theme);
  }, [theme]);

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
              onClick={() => setActiveId(book.id)}
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
            rehypePlugins={[rehypeHighlight]}
            components={{
              pre({ children }) {
                if (
                  isValidElement<{ className?: string; children?: unknown }>(children) &&
                  typeof children.props.className === "string" &&
                  children.props.className.includes("language-mermaid")
                ) {
                  const chart = String(children.props.children).replace(/\n$/, "");
                  return <MermaidBlock chart={chart} theme={theme} />;
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
