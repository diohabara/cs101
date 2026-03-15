import { Fragment, useEffect, useMemo, useState } from "react";
import type { StageQuiz } from "../quiz";

type QuizCardProps = {
  stageId: string;
  quiz: StageQuiz;
};

type StoredQuizState = {
  version: number;
  answers: Record<string, string[]>;
  submitted: boolean;
};

type QuestionResult = {
  questionId: string;
  isCorrect: boolean;
};

const STORAGE_VERSION = 2;

function renderInlineCodeText(text: string) {
  return text.split(/(`[^`]+`)/g).map((part, index) => {
    if (part.startsWith("`") && part.endsWith("`") && part.length >= 2) {
      return <code key={index}>{part.slice(1, -1)}</code>;
    }

    return <Fragment key={index}>{part}</Fragment>;
  });
}

function storageKey(stageId: string) {
  return `cs101-quiz:${stageId}`;
}

function isStoredQuizState(value: unknown): value is StoredQuizState {
  if (!value || typeof value !== "object") {
    return false;
  }

  const candidate = value as Partial<StoredQuizState>;
  if (candidate.version !== STORAGE_VERSION || typeof candidate.submitted !== "boolean") {
    return false;
  }

  if (!candidate.answers || typeof candidate.answers !== "object") {
    return false;
  }

  return Object.values(candidate.answers).every(
    (entry) => Array.isArray(entry) && entry.every((item) => typeof item === "string")
  );
}

function answersMatch(expected: string[], actual: string[]) {
  if (expected.length !== actual.length) {
    return false;
  }

  const actualSet = new Set(actual);
  return expected.every((id) => actualSet.has(id));
}

function scoreQuiz(quiz: StageQuiz, answers: Record<string, string[]>) {
  const results: QuestionResult[] = quiz.questions.map((question) => {
    const expected = question.options.filter((option) => option.isCorrect).map((option) => option.id);
    const selected = answers[question.id] ?? [];
    return {
      questionId: question.id,
      isCorrect: answersMatch(expected, selected),
    };
  });

  const correctCount = results.filter((result) => result.isCorrect).length;
  const totalCount = quiz.questions.length;

  return {
    results,
    correctCount,
    totalCount,
    status:
      correctCount === totalCount
        ? "all_correct"
        : correctCount > 0
          ? "partially_correct"
          : "needs_review",
  } as const;
}

export function QuizCard({ stageId, quiz }: QuizCardProps) {
  const [answers, setAnswers] = useState<Record<string, string[]>>({});
  const [submitted, setSubmitted] = useState(false);
  const [isHydrated, setIsHydrated] = useState(false);

  useEffect(() => {
    if (typeof window === "undefined") {
      return;
    }

    try {
      const raw = window.localStorage.getItem(storageKey(stageId));
      if (!raw) {
        setAnswers({});
        setSubmitted(false);
        setIsHydrated(true);
        return;
      }

      const parsed: unknown = JSON.parse(raw);
      if (!isStoredQuizState(parsed)) {
        setAnswers({});
        setSubmitted(false);
        setIsHydrated(true);
        return;
      }

      setAnswers(parsed.answers);
      setSubmitted(parsed.submitted);
      setIsHydrated(true);
    } catch {
      setAnswers({});
      setSubmitted(false);
      setIsHydrated(true);
    }
  }, [stageId]);

  useEffect(() => {
    if (typeof window === "undefined" || !isHydrated) {
      return;
    }

    const payload: StoredQuizState = {
      version: STORAGE_VERSION,
      answers,
      submitted,
    };

    window.localStorage.setItem(storageKey(stageId), JSON.stringify(payload));
  }, [answers, isHydrated, stageId, submitted]);

  const score = useMemo(() => scoreQuiz(quiz, answers), [answers, quiz]);
  const answeredCount = useMemo(
    () => quiz.questions.filter((question) => (answers[question.id] ?? []).length > 0).length,
    [answers, quiz]
  );

  function handleSingleChoice(questionId: string, optionId: string) {
    setAnswers((current) => ({
      ...current,
      [questionId]: [optionId],
    }));
  }

  function handleMultipleChoice(questionId: string, optionId: string, checked: boolean) {
    setAnswers((current) => {
      const previous = current[questionId] ?? [];
      const next = checked
        ? [...previous, optionId]
        : previous.filter((value) => value !== optionId);

      return {
        ...current,
        [questionId]: next,
      };
    });
  }

  function handleSubmit() {
    setSubmitted(true);
  }

  function handleReset() {
    setAnswers({});
    setSubmitted(false);
  }

  return (
    <section className="quiz-card" aria-labelledby={`quiz-title-${stageId}`}>
      <div className="quiz-card__header">
        <div>
          <p className="quiz-card__eyebrow">Chapter Quiz</p>
          <h3 id={`quiz-title-${stageId}`}>{quiz.title}</h3>
          <p className="quiz-rich-text">{renderInlineCodeText(quiz.intro)}</p>
        </div>
        <div className="quiz-card__meta">
          <span>{answeredCount} / {quiz.questions.length} 問 回答済み</span>
          {submitted ? (
            <span className={`quiz-card__status quiz-card__status--${score.status}`}>
              {score.correctCount} / {score.totalCount} 正解
            </span>
          ) : null}
        </div>
      </div>

      <div className="quiz-card__questions">
        {quiz.questions.map((question, questionIndex) => {
          const selected = answers[question.id] ?? [];
          const questionResult = score.results.find((result) => result.questionId === question.id);

          return (
            <fieldset key={question.id} className="quiz-question">
              <legend>
                <span className="quiz-question__number">Q{questionIndex + 1}</span>
                <span className="quiz-rich-text">{renderInlineCodeText(question.prompt)}</span>
              </legend>
              <p className="quiz-question__type">
                {question.type === "single" ? "単一選択" : "複数選択"}
              </p>

              <div className="quiz-question__options">
                {question.options.map((option) => {
                  const checked = selected.includes(option.id);

                  return (
                    <label key={option.id} className="quiz-option">
                      <input
                        checked={checked}
                        name={question.id}
                        onChange={(event) => {
                          if (question.type === "single") {
                            handleSingleChoice(question.id, option.id);
                            return;
                          }

                          handleMultipleChoice(question.id, option.id, event.target.checked);
                        }}
                        type={question.type === "single" ? "radio" : "checkbox"}
                      />
                      <span className="quiz-rich-text">{renderInlineCodeText(option.label)}</span>
                    </label>
                  );
                })}
              </div>

              {submitted && questionResult ? (
                <div
                  className={
                    questionResult.isCorrect
                      ? "quiz-question__feedback quiz-question__feedback--correct"
                      : "quiz-question__feedback quiz-question__feedback--review"
                  }
                >
                  <strong>{questionResult.isCorrect ? "正解" : "要復習"}</strong>
                  <p className="quiz-rich-text">{renderInlineCodeText(question.explanation)}</p>
                </div>
              ) : null}
            </fieldset>
          );
        })}
      </div>

      <div className="quiz-card__actions">
        <button className="quiz-button quiz-button--primary" onClick={handleSubmit} type="button">
          採点する
        </button>
        <button className="quiz-button quiz-button--secondary" onClick={handleReset} type="button">
          回答をリセット
        </button>
      </div>

      {submitted ? (
        <div className={`quiz-summary quiz-summary--${score.status}`}>
          <h4>結果</h4>
          <p className="quiz-rich-text">{renderInlineCodeText(quiz.resultMessages[score.status])}</p>
        </div>
      ) : null}
    </section>
  );
}
