type Topic = "cpu" | "os" | "compiler";

type TopicBadgeProps = {
  topic: Topic;
};

export function TopicBadge({ topic }: TopicBadgeProps) {
  const label = topic.toUpperCase();
  const icon =
    topic === "cpu" ? (
      <>
        <rect x="6.5" y="6.5" width="11" height="11" rx="2.5" />
        <path d="M9.5 1.75v3M14.5 1.75v3M9.5 19.25v3M14.5 19.25v3M1.75 9.5h3M1.75 14.5h3M19.25 9.5h3M19.25 14.5h3" />
        <path d="M10 10h4v4h-4z" />
      </>
    ) : topic === "os" ? (
      <>
        <rect x="4.5" y="5.5" width="15" height="13" rx="3" />
        <path d="M4.5 9.5h15" />
        <circle cx="8" cy="7.5" r="0.8" fill="currentColor" stroke="none" />
        <circle cx="11" cy="7.5" r="0.8" fill="currentColor" stroke="none" />
        <path d="M8 13h8M8 16h5" />
      </>
    ) : (
      <>
        <circle cx="6.5" cy="7" r="2.25" />
        <circle cx="17.5" cy="7" r="2.25" />
        <circle cx="12" cy="17" r="2.6" />
        <path d="M8.4 8.5 10.8 14M15.6 8.5 13.2 14" />
      </>
    );

  return (
    <span className={`topic-badge topic-badge--${topic}`} aria-label={label} title={label}>
      <svg viewBox="0 0 24 24" aria-hidden="true">
        {icon}
      </svg>
    </span>
  );
}
