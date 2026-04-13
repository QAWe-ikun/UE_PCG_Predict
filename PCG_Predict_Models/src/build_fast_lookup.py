"""
build_fast_lookup.py
从 data/samples.jsonl 构建 FastPredictor 的 n-gram 统计查找表，
输出到 models/fast_lookup.json。

查找表格式:
  {
    "version": "1.0",
    "total_samples": 5000,
    "unigram":  {"87": {"36": 0.45, "100": 0.20}},
    "bigram":   {"87,36": {"100": 0.72}},
    "trigram":  {"87,36,100": {"7": 0.80}},
    "starter":  {"87": 0.35, "55": 0.25}
  }

key 为逗号分隔的 node_type_id 字符串（最近 N 个节点），
每个 key 只保留 Top-K 候选（默认 20）。
"""

import json
from collections import defaultdict
from pathlib import Path


BASE_DIR      = Path(__file__).parent.parent
SAMPLES_PATH  = BASE_DIR / "data" / "samples.jsonl"
OUTPUT_PATH   = BASE_DIR / "models" / "fast_lookup.json"
REGISTRY_PATH = BASE_DIR / "config" / "node_registry.json"

TOP_K = 20


def _load_all_node_ids(registry_path: Path = REGISTRY_PATH) -> list[int]:
    """返回注册表中所有节点 ID 的有序列表。"""
    if not registry_path.exists():
        return []
    with open(registry_path, encoding="utf-8") as f:
        registry = json.load(f)
    nodes = registry if isinstance(registry, list) else registry.get("nodes", [])
    return sorted(int(n["id"]) for n in nodes if "id" in n)


def build_fast_lookup(
    samples_path: Path = SAMPLES_PATH,
    output_path: Path  = OUTPUT_PATH,
    registry_path: Path = REGISTRY_PATH,
    top_k: int         = TOP_K,
    verbose: bool      = True,
) -> dict:
    """
    读取 samples.jsonl，统计 n-gram 转移概率，写出查找表 JSON。
    返回构建好的查找表字典。
    """
    # 计数器
    unigram_counts: dict[str, dict[str, int]] = defaultdict(lambda: defaultdict(int))
    bigram_counts:  dict[str, dict[str, int]] = defaultdict(lambda: defaultdict(int))
    trigram_counts: dict[str, dict[str, int]] = defaultdict(lambda: defaultdict(int))
    starter_counts: dict[str, int]            = defaultdict(int)
    total = 0

    if not samples_path.exists():
        print(f"[build_fast_lookup] samples file not found: {samples_path}")
        print("  Run data_collector.py first to generate samples.")
        return {}

    with open(samples_path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                sample = json.loads(line)
            except json.JSONDecodeError:
                continue

            history: list[int] = sample.get("history_sequence", [])
            target:  int       = sample.get("target_node_id", -1)
            if target < 0:
                continue

            total += 1
            t_str = str(target)

            # starter：空历史（第一个节点）
            if len(history) == 0:
                starter_counts[t_str] += 1
                continue

            # unigram：最近 1 个节点
            unigram_counts[str(history[-1])][t_str] += 1

            # bigram：最近 2 个节点
            if len(history) >= 2:
                key = f"{history[-2]},{history[-1]}"
                bigram_counts[key][t_str] += 1

            # trigram：最近 3 个节点
            if len(history) >= 3:
                key = f"{history[-3]},{history[-2]},{history[-1]}"
                trigram_counts[key][t_str] += 1

    def normalize_topk(counts: dict[str, dict[str, int]], k: int) -> dict:
        result = {}
        for ctx_key, target_counts in counts.items():
            total_ctx = sum(target_counts.values())
            if total_ctx == 0:
                continue
            sorted_items = sorted(target_counts.items(), key=lambda x: x[1], reverse=True)[:k]
            result[ctx_key] = {t: round(c / total_ctx, 4) for t, c in sorted_items}
        return result

    # 归一化 starter
    starter_total = sum(starter_counts.values())
    starter_normalized: dict[str, float] = {}
    if starter_total > 0:
        sorted_starter = sorted(starter_counts.items(), key=lambda x: x[1], reverse=True)[:top_k]
        starter_normalized = {k: round(v / starter_total, 4) for k, v in sorted_starter}

    lookup = {
        "version":       "1.0",
        "total_samples": total,
        "unigram":       normalize_topk(unigram_counts, top_k),
        "bigram":        normalize_topk(bigram_counts,  top_k),
        "trigram":       normalize_topk(trigram_counts, top_k),
        "starter":       starter_normalized,
    }

    # 确保 unigram 表覆盖注册表中每个节点
    # 对训练数据中未出现的节点，补充均匀分布 fallback（所有其他节点等概率）
    all_node_ids = _load_all_node_ids(registry_path)
    if all_node_ids:
        n = len(all_node_ids)
        uniform_prob = round(1.0 / n, 6)
        fallback_entry = {str(nid): uniform_prob for nid in all_node_ids}
        added = 0
        for nid in all_node_ids:
            key = str(nid)
            if key not in lookup["unigram"]:
                lookup["unigram"][key] = fallback_entry
                added += 1
        if verbose and added:
            print(f"  unigram fallback: added {added} nodes with uniform distribution")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(lookup, f, indent=2, ensure_ascii=False)

    if verbose:
        print(f"[build_fast_lookup] {total} samples processed")
        print(f"  unigram keys:  {len(lookup['unigram'])}")
        print(f"  bigram keys:   {len(lookup['bigram'])}")
        print(f"  trigram keys:  {len(lookup['trigram'])}")
        print(f"  starter nodes: {len(lookup['starter'])}")
        print(f"  Output: {output_path}")

    return lookup


if __name__ == "__main__":
    build_fast_lookup()
