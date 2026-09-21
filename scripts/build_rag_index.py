#!/usr/bin/env python3
"""
build_rag_index.py — RAG Knowledge Base Indexer for vibeKidbright IDE
======================================================================
Reads all .md/.txt/.c/.h files from knowledge_base/, chunks them using
sentence-boundary splitting, generates vector embeddings, and writes
.embeddings.json in the exact format the Rust backend (ai_chat.rs) reads.

JSON schema produced:
{
  "chunks": [
    {"file_name": "relative/path.md", "content": "...", "embedding": [0.1, -0.2, ...]},
    ...
  ],
  "last_indexed": {
    "relative/path.md": <unix_timestamp_secs>,
    ...
  }
}

Usage:
  # With OpenAI (recommended for best quality)
  python scripts/build_rag_index.py --openai-key sk-...

  # With local LM Studio / Ollama (free, no API key needed)
  python scripts/build_rag_index.py --local http://localhost:1234

  # With open-source sentence-transformers (offline, no API)
  python scripts/build_rag_index.py --local-model

  # Force re-index all files (ignore timestamps)
  python scripts/build_rag_index.py --openai-key sk-... --force

  # Preview chunks only (no embedding)
  python scripts/build_rag_index.py --dry-run

Requirements:
  pip install openai requests tqdm
  pip install sentence-transformers  # only if using --local-model
"""

import os
import sys
import json
import time
import argparse
from pathlib import Path
from typing import Optional

# Force UTF-8 output so emoji don't break on Thai Windows terminals (cp874)
if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

# ── Constants (mirror Rust values in ai_chat.rs) ──────────────────────────────
CHUNK_TARGET_SIZE = 800   # characters per chunk
CHUNK_OVERLAP     = 100   # overlap characters between consecutive chunks
SUPPORTED_EXTS    = {".md", ".txt", ".c", ".h"}
INDEX_FILE        = ".embeddings.json"
OPENAI_MODEL      = "text-embedding-3-small"  # 1536-dim, cheap, fast


# ══════════════════════════════════════════════════════════════════════════════
#  SECTION 1: TEXT CHUNKING
#  Mirrors the sentence-boundary chunk_text() function in ai_chat.rs
# ══════════════════════════════════════════════════════════════════════════════

def chunk_text(text: str, target_size: int = CHUNK_TARGET_SIZE, overlap: int = CHUNK_OVERLAP) -> list[str]:
    """
    Split text into overlapping chunks at sentence boundaries.
    Matches the Rust chunk_text() logic in ai_chat.rs exactly.
    
    Strategy:
      1. Split on '. ', '! ', '? ', or newlines to get sentences
      2. Group sentences until we reach target_size
      3. Carry the last `overlap` chars into the next chunk
    """
    if not text.strip():
        return []

    # ── Step 1: Find sentence boundaries ──────────────────────────────────────
    sentences = []
    last = 0
    i = 0
    while i < len(text):
        ch = text[i]
        if ch in '.!?':
            # Check if followed by space or newline (real sentence end)
            next_ch = text[i + 1] if i + 1 < len(text) else ''
            if next_ch in (' ', '\n', ''):
                sentences.append(text[last:i + 1])
                last = i + 1
        i += 1
    # Remainder after last sentence terminator
    if last < len(text):
        sentences.append(text[last:])

    if not sentences:
        # Fallback: no sentence terminators found, split by newlines
        sentences = text.split('\n')
        sentences = [s for s in sentences if s.strip()]

    # ── Step 2: Group sentences into chunks with overlap ──────────────────────
    chunks = []
    current = ""
    overlap_buf = ""

    for sentence in sentences:
        if len(current) + len(sentence) > target_size and current:
            chunks.append(current)
            # Carry tail for overlap
            tail_start = max(0, len(current) - overlap)
            overlap_buf = current[tail_start:]
            current = overlap_buf + " "
        current += sentence

    if current.strip():
        chunks.append(current)

    # Absolute fallback
    if not chunks and text:
        chunks = [text[:target_size]]

    return chunks


# ══════════════════════════════════════════════════════════════════════════════
#  SECTION 2: FILE DISCOVERY
#  Mirrors collect_kb_files_inner() in ai_chat.rs
# ══════════════════════════════════════════════════════════════════════════════

def collect_kb_files(kb_path: Path) -> list[tuple[Path, str]]:
    """
    Recursively collect supported files from knowledge_base/.
    Returns list of (absolute_path, relative_key) pairs.
    relative_key uses forward slashes (e.g. "sensor_examples/accel_kxtj3.c")
    """
    results = []
    for filepath in sorted(kb_path.rglob("*")):
        # Skip hidden files (like .embeddings.json)
        if any(part.startswith('.') for part in filepath.parts):
            continue
        if filepath.is_file() and filepath.suffix.lower() in SUPPORTED_EXTS:
            rel_key = filepath.relative_to(kb_path).as_posix()
            results.append((filepath, rel_key))
    return results


# ══════════════════════════════════════════════════════════════════════════════
#  SECTION 3: EMBEDDING BACKENDS
# ══════════════════════════════════════════════════════════════════════════════

class EmbeddingBackend:
    """Abstract base class for embedding backends."""
    def embed(self, text: str) -> list[float]:
        raise NotImplementedError

    def name(self) -> str:
        raise NotImplementedError


class OpenAIBackend(EmbeddingBackend):
    """
    Uses OpenAI's text-embedding-3-small model.
    Cost: ~$0.02 per 1M tokens (extremely cheap for KB indexing).
    Dimension: 1536
    """
    def __init__(self, api_key: str, base_url: str = "https://api.openai.com/v1"):
        import openai
        self.client = openai.OpenAI(api_key=api_key, base_url=base_url)

    def embed(self, text: str) -> list[float]:
        response = self.client.embeddings.create(
            model=OPENAI_MODEL,
            input=text
        )
        return response.data[0].embedding

    def name(self) -> str:
        return f"OpenAI ({OPENAI_MODEL})"


class LocalAPIBackend(EmbeddingBackend):
    """
    Uses any local OpenAI-compatible API (LM Studio, Ollama, llama.cpp server).
    Works with any model that supports /v1/embeddings endpoint.
    
    Popular local embedding models:
      - nomic-embed-text (768-dim, fast)
      - all-minilm (384-dim, very fast)
      - mxbai-embed-large (1024-dim, high quality)
    """
    def __init__(self, base_url: str, model: str = "nomic-embed-text"):
        import requests
        self.session = requests.Session()
        self.base_url = base_url.rstrip('/').rstrip('/v1')
        if not self.base_url.endswith('/v1'):
            self.base_url += '/v1'
        self.model = model

    def embed(self, text: str) -> list[float]:
        import requests
        resp = self.session.post(
            f"{self.base_url}/embeddings",
            json={"model": self.model, "input": text},
            timeout=60
        )
        resp.raise_for_status()
        data = resp.json()
        if "error" in data:
            raise ValueError(f"API Error: {data['error']}")
        return data["data"][0]["embedding"]

    def name(self) -> str:
        return f"Local API ({self.base_url}, model={self.model})"


class SentenceTransformersBackend(EmbeddingBackend):
    """
    Uses sentence-transformers library for fully offline embedding.
    No API key, no internet needed after first download.
    
    Recommended models (install: pip install sentence-transformers):
      - 'all-MiniLM-L6-v2'         → 384-dim, very fast (default)
      - 'all-mpnet-base-v2'         → 768-dim, better quality
      - 'BAAI/bge-small-en-v1.5'    → 384-dim, best small model
      - 'intfloat/multilingual-e5-large' → multi-language support
    """
    def __init__(self, model_name: str = "all-MiniLM-L6-v2"):
        print(f"  Loading model '{model_name}' (first run may download ~80MB)...")
        try:
            from sentence_transformers import SentenceTransformer
            self.model = SentenceTransformer(model_name)
            self.model_name = model_name
        except ImportError:
            print("ERROR: sentence-transformers not installed.")
            print("  Run: pip install sentence-transformers")
            sys.exit(1)

    def embed(self, text: str) -> list[float]:
        embedding = self.model.encode(text, normalize_embeddings=True)
        return embedding.tolist()

    def name(self) -> str:
        return f"SentenceTransformers ({self.model_name})"


# ══════════════════════════════════════════════════════════════════════════════
#  SECTION 4: MAIN INDEXER
# ══════════════════════════════════════════════════════════════════════════════

def load_index(index_path: Path) -> dict:
    """Load existing .embeddings.json or return a fresh empty index."""
    if index_path.exists():
        try:
            with open(index_path, 'r', encoding='utf-8') as f:
                data = json.load(f)
            # Validate structure
            if "chunks" in data and "last_indexed" in data:
                return data
        except (json.JSONDecodeError, KeyError):
            print(f"  ⚠ Warning: corrupted {INDEX_FILE}, starting fresh.")
    return {"chunks": [], "last_indexed": {}}


def save_index(index_path: Path, index: dict) -> None:
    """Write index to .embeddings.json in pretty-printed format."""
    with open(index_path, 'w', encoding='utf-8') as f:
        json.dump(index, f, ensure_ascii=False, indent=2)
    size_kb = index_path.stat().st_size / 1024
    print(f"\n  [saved] {index_path} ({size_kb:.1f} KB)")


def get_mtime(filepath: Path) -> int:
    """Return file modification time as Unix timestamp (seconds)."""
    return int(filepath.stat().st_mtime)


def cosine_similarity(v1: list[float], v2: list[float]) -> float:
    """Compute cosine similarity between two vectors (for testing)."""
    dot = sum(a * b for a, b in zip(v1, v2))
    n1 = sum(a * a for a in v1) ** 0.5
    n2 = sum(b * b for b in v2) ** 0.5
    return dot / (n1 * n2) if n1 > 0 and n2 > 0 else 0.0


def build_index(
    kb_path: Path,
    backend,
    force: bool = False,
    dry_run: bool = False,
    verbose: bool = False,
    chunk_size: int = CHUNK_TARGET_SIZE,
    overlap: int = CHUNK_OVERLAP,
) -> dict:
    """
    Main indexing loop:
      1. Discover all KB files
      2. Skip files that haven't changed (unless --force)
      3. Chunk each file
      4. Generate embeddings for each chunk
      5. Update the index dict
      6. Return updated index
    """
    index_path = kb_path / INDEX_FILE
    index = load_index(index_path)

    all_files = collect_kb_files(kb_path)
    if not all_files:
        print(f"  No supported files found in {kb_path}")
        return index

    print(f"\n  Found {len(all_files)} file(s) in knowledge_base/")
    if backend:
        print(f"  Embedding backend: {backend.name()}")
    print(f"  Chunk size: {chunk_size} chars, overlap: {overlap} chars")
    print()

    stats = {"skipped": 0, "indexed": 0, "chunks_added": 0, "errors": 0}

    for filepath, rel_key in all_files:
        mtime = get_mtime(filepath)
        cached_mtime = index["last_indexed"].get(rel_key, 0)

        # Skip unchanged files (unless --force)
        if not force and cached_mtime >= mtime:
            if verbose:
                print(f"  [skip] unchanged: {rel_key}")
            stats["skipped"] += 1
            continue

        print(f"  [+] Indexing: {rel_key}")

        # Read file content
        try:
            content = filepath.read_text(encoding='utf-8', errors='replace')
        except Exception as e:
            print(f"     [!] Read error: {e}")
            stats["errors"] += 1
            continue

        if not content.strip():
            print(f"     [empty] Skipping.")
            index["last_indexed"][rel_key] = mtime
            continue

        # Chunk the content
        chunks = chunk_text(content, chunk_size, overlap)
        print(f"     → {len(chunks)} chunk(s)", end="")

        if dry_run:
            print(f" [DRY RUN — no embedding]")
            for i, chunk in enumerate(chunks):
                if verbose:
                    preview = chunk[:120].replace('\n', ' ')
                    print(f"       Chunk {i+1}: {preview}...")
            stats["indexed"] += 1
            stats["chunks_added"] += len(chunks)
            continue

        if backend is None:
            print()
            continue

        # Generate embeddings for each chunk
        new_chunks = []
        chunk_errors = 0
        for chunk_text_content in chunks:
            try:
                embedding = backend.embed(chunk_text_content)
                new_chunks.append({
                    "file_name": rel_key,
                    "content": chunk_text_content,
                    "embedding": embedding
                })
                print(".", end="", flush=True)
                # Rate limit protection for OpenAI free tier
                time.sleep(0.05)
            except Exception as e:
                print(f"\n     [!] Embedding error: {e}")
                chunk_errors += 1
                if chunk_errors > 3:
                    print(f"     [!!] Too many errors, skipping file.")
                    break

        print()  # newline after dots

        if new_chunks:
            # Remove old chunks for this file, add new ones
            index["chunks"] = [
                c for c in index["chunks"]
                if c["file_name"] != rel_key
            ]
            index["chunks"].extend(new_chunks)
            index["last_indexed"][rel_key] = mtime
            stats["indexed"] += 1
            stats["chunks_added"] += len(new_chunks)
            print(f"     [OK] {len(new_chunks)} chunk(s) embedded")
        else:
            print(f"     [!!] No chunks embedded, keeping old index for this file")
            stats["errors"] += 1

    return index, stats


# ══════════════════════════════════════════════════════════════════════════════
#  SECTION 5: VECTOR SEARCH (for testing)
# ══════════════════════════════════════════════════════════════════════════════

def search(index: dict, backend: EmbeddingBackend, query: str, top_k: int = 5) -> list[dict]:
    """
    Perform cosine similarity search against the index.
    Returns top_k most relevant chunks.
    """
    if not index["chunks"]:
        return []

    query_embedding = backend.embed(query)

    scored = []
    for chunk in index["chunks"]:
        score = cosine_similarity(query_embedding, chunk["embedding"])
        scored.append({
            "score": score,
            "file": chunk["file_name"],
            "content": chunk["content"][:300] + "..." if len(chunk["content"]) > 300 else chunk["content"]
        })

    scored.sort(key=lambda x: x["score"], reverse=True)
    return scored[:top_k]


# ══════════════════════════════════════════════════════════════════════════════
#  SECTION 6: CLI ENTRY POINT
# ══════════════════════════════════════════════════════════════════════════════

def main():
    global CHUNK_TARGET_SIZE, CHUNK_OVERLAP  # declared early to satisfy Python's scoping rules
    parser = argparse.ArgumentParser(
        description="Build RAG vector index for vibeKidbright knowledge_base/",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # OpenAI (best quality, requires API key)
  python scripts/build_rag_index.py --openai-key sk-...

  # Local LM Studio on port 1234
  python scripts/build_rag_index.py --local http://localhost:1234 --local-model-name nomic-embed-text

  # Fully offline with sentence-transformers
  python scripts/build_rag_index.py --sentence-transformers

  # Force reindex everything
  python scripts/build_rag_index.py --openai-key sk-... --force

  # Preview chunks without generating embeddings
  python scripts/build_rag_index.py --dry-run --verbose

  # Test search after indexing
  python scripts/build_rag_index.py --openai-key sk-... --query "LM73 temperature sensor I2C"
        """
    )

    # Backend selection
    backend_group = parser.add_mutually_exclusive_group()
    backend_group.add_argument("--openai-key", metavar="API_KEY",
                               help="OpenAI API key (uses text-embedding-3-small)")
    backend_group.add_argument("--local", metavar="URL",
                               help="Local OpenAI-compatible API URL (e.g. http://localhost:1234)")
    backend_group.add_argument("--sentence-transformers", action="store_true",
                               help="Use sentence-transformers library (fully offline)")

    # Options
    parser.add_argument("--kb-path", default=None,
                        help="Path to knowledge_base/ folder (auto-detected if not set)")
    parser.add_argument("--local-model-name", default="nomic-embed-text",
                        help="Model name for local API (default: nomic-embed-text)")
    parser.add_argument("--st-model", default="all-MiniLM-L6-v2",
                        help="sentence-transformers model name (default: all-MiniLM-L6-v2)")
    parser.add_argument("--force", action="store_true",
                        help="Re-index all files even if unchanged")
    parser.add_argument("--dry-run", action="store_true",
                        help="Preview chunks without generating embeddings")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Show detailed output per chunk")
    parser.add_argument("--query", metavar="TEXT",
                        help="Run a test search query after indexing")
    parser.add_argument("--top-k", type=int, default=5,
                        help="Number of search results to show (default: 5)")
    parser.add_argument("--chunk-size", type=int, default=CHUNK_TARGET_SIZE,
                        help=f"Characters per chunk (default: {CHUNK_TARGET_SIZE})")
    parser.add_argument("--overlap", type=int, default=CHUNK_OVERLAP,
                        help=f"Overlap characters between chunks (default: {CHUNK_OVERLAP})")

    args = parser.parse_args()

    # ── Locate knowledge_base/ ─────────────────────────────────────────────────
    if args.kb_path:
        kb_path = Path(args.kb_path)
    else:
        # Auto-detect: look for knowledge_base/ relative to this script
        script_dir = Path(__file__).parent
        candidates = [
            script_dir.parent / "knowledge_base",   # vibeKidbright/knowledge_base
            script_dir / "knowledge_base",           # scripts/knowledge_base
            Path.cwd() / "knowledge_base",           # CWD/knowledge_base
        ]
        kb_path = next((p for p in candidates if p.is_dir()), None)
        if kb_path is None:
            print("ERROR: Could not find knowledge_base/ directory.")
            print("  Use --kb-path to specify the path manually.")
            sys.exit(1)

    print(f"\n{'='*60}")
    print(f"  vibeKidbright RAG Indexer")
    print(f"{'='*60}")
    print(f"  KB path: {kb_path.resolve()}")

    # ── Build embedding backend ────────────────────────────────────────────────
    backend = None
    if args.dry_run:
        print("  Mode: DRY RUN (no embeddings)")
    elif args.openai_key:
        try:
            import openai
        except ImportError:
            print("ERROR: openai package not installed. Run: pip install openai")
            sys.exit(1)
        backend = OpenAIBackend(api_key=args.openai_key)
    elif args.local:
        try:
            import requests
        except ImportError:
            print("ERROR: requests package not installed. Run: pip install requests")
            sys.exit(1)
        backend = LocalAPIBackend(base_url=args.local, model=args.local_model_name)
    elif args.sentence_transformers:
        backend = SentenceTransformersBackend(model_name=args.st_model)
    else:
        print("  ⚠ No embedding backend specified.")
        print("  Using --dry-run mode (chunk preview only).")
        print("  To embed, use --openai-key, --local, or --sentence-transformers\n")
        args.dry_run = True

    # Override chunk params if specified
    CHUNK_TARGET_SIZE = args.chunk_size
    CHUNK_OVERLAP = args.overlap

    # ── Run indexing ───────────────────────────────────────────────────────────
    start_time = time.time()
    index, stats = build_index(
        kb_path=kb_path,
        backend=backend,
        force=args.force,
        dry_run=args.dry_run,
        verbose=args.verbose,
        chunk_size=args.chunk_size,
        overlap=args.overlap,
    )

    elapsed = time.time() - start_time

    # ── Save index (unless dry run) ────────────────────────────────────────────
    if not args.dry_run:
        save_index(kb_path / INDEX_FILE, index)

    # ── Print summary ──────────────────────────────────────────────────────────
    total_chunks = len(index["chunks"])
    print(f"\n{'='*60}")
    print(f"  [OK] Indexing complete in {elapsed:.1f}s")
    print(f"  Files indexed:  {stats['indexed']}")
    print(f"  Files skipped:  {stats['skipped']} (unchanged)")
    print(f"  Chunks added:   {stats['chunks_added']}")
    print(f"  Total chunks:   {total_chunks}")
    print(f"  Errors:         {stats['errors']}")
    if total_chunks > 0 and index["chunks"]:
        dim = len(index["chunks"][0]["embedding"]) if index["chunks"][0]["embedding"] else 0
        print(f"  Embedding dim:  {dim}")
    print(f"{'='*60}")

    # ── Optional test search ───────────────────────────────────────────────────
    if args.query and backend and not args.dry_run:
        print(f"\n  [search] Test Search: \"{args.query}\"")
        print(f"  {'─'*56}")
        results = search(index, backend, args.query, top_k=args.top_k)
        for i, result in enumerate(results, 1):
            print(f"\n  [{i}] Score: {result['score']:.4f}  |  File: {result['file']}")
            print(f"      {result['content'][:200].replace(chr(10), ' ')}")


if __name__ == "__main__":
    main()
