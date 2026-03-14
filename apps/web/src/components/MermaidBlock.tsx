import { useMemo } from "react";
import { renderMermaidSVG } from "beautiful-mermaid";

type MermaidBlockProps = {
  chart: string;
  theme: "light" | "dark";
};

export function MermaidBlock({ chart, theme }: MermaidBlockProps) {
  const rendered = useMemo(() => {
    try {
      return {
        svg: renderMermaidSVG(chart, {
          bg: "var(--paper-strong)",
          fg: "var(--text)",
          line: "var(--line-strong)",
          accent: "var(--accent)",
          muted: "var(--muted)",
          surface: "var(--accent-soft)",
          border: "var(--line-strong)",
          font: "\"Avenir Next\", \"Hiragino Sans\", sans-serif",
          transparent: true,
          padding: 24,
          nodeSpacing: 28,
          layerSpacing: 38,
          componentSpacing: 24,
        }),
        error: null,
      };
    } catch (renderError) {
      return {
        svg: "",
        error: renderError instanceof Error ? renderError.message : "unknown error",
      };
    }
  }, [chart, theme]);

  if (rendered.error) {
    return (
      <div className="mermaid-error">
        <p>Mermaid の描画に失敗しました。</p>
        <code>{rendered.error}</code>
        <pre>
          <code>{chart}</code>
        </pre>
      </div>
    );
  }

  return <div className="mermaid-card" dangerouslySetInnerHTML={{ __html: rendered.svg }} />;
}
