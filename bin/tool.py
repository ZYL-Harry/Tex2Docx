import os
import subprocess
import tempfile
import shutil
import zipfile
from pathlib import Path
import time

# 项目基础路径（可根据实际情况调整）
BASE_DIR = Path(__file__).parent.parent  # 假设 tools.py 在 app 目录下，BASE_DIR 为项目根目录
DEFAULT_BIB = BASE_DIR / 'data' / 'default' / 'refs.bib'
DEFAULT_CLS = BASE_DIR / 'data' / 'default' / 'article.cls'
DEFAULT_FIGURES_ZIP = BASE_DIR / 'data' / 'default' / 'figures.zip'
CPP_EXE = BASE_DIR / 'app' / 'Project1.exe'  # 根据实际路径修改


def convert_latex(tex_content: str) -> str:
    """
    将 LaTeX 源码转换为 Word 文档，返回生成的 docx 文件路径。
    使用默认的 bib、cls、图片资源。
    """
    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir = Path(tmpdir)
        # 1. 保存 tex 文件
        tex_path = tmpdir / 'document.tex'
        tex_path.write_text(tex_content, encoding='utf-8')

        # 2. 处理图片目录（解压默认图片 zip 到 tmpdir/figures）
        img_dir = tmpdir / 'figures'
        img_dir.mkdir()
        if DEFAULT_FIGURES_ZIP.exists():
            with zipfile.ZipFile(DEFAULT_FIGURES_ZIP, 'r') as zip_ref:
                zip_ref.extractall(img_dir)
        else:
            raise FileNotFoundError(f"默认图片压缩包不存在: {DEFAULT_FIGURES_ZIP}")

        # 3. 复制 bib 和 cls 到临时目录（可选，但避免路径问题）
        bib_path = tmpdir / 'refs.bib'
        shutil.copy(DEFAULT_BIB, bib_path)
        cls_path = tmpdir / 'article.cls'
        shutil.copy(DEFAULT_CLS, cls_path)

        # 4. 输出文件路径
        output_path = tmpdir / 'output.docx'

        # 5. 调用 C++ 程序
        cmd = [
            str(CPP_EXE),
            str(tex_path),
            str(bib_path),
            str(img_dir) + '\\',  # 注意 Windows 路径末尾需要反斜杠
            str(cls_path),
            str(output_path)
        ]
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
        if result.returncode != 0:
            raise RuntimeError(f"转换失败: {result.stderr}")

        # 6. 将生成的文件复制到持久化目录（避免临时目录删除后丢失）
        output_dir = BASE_DIR / 'data' / 'outputs'
        output_dir.mkdir(parents=True, exist_ok=True)
        final_path = output_dir / f"tex2docx_{int(time.time())}.docx"
        shutil.copy(output_path, final_path)

        return str(final_path)


import os
import zipfile
import tempfile
import shutil
import xml.etree.ElementTree as ET
from pathlib import Path


def polish_docx(docx_path: str):
    """
    对 Word 文档进行智能校对与美化（通过直接操作 XML）。
    返回优化后的文件路径。
    """
    # 1. 创建临时目录
    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir = Path(tmpdir)
        # 2. 解压 docx 到临时目录
        with zipfile.ZipFile(docx_path, 'r') as zip_ref:
            zip_ref.extractall(tmpdir)

        # 3. 定位 document.xml
        doc_xml_path = tmpdir / 'word' / 'document.xml'
        if not doc_xml_path.exists():
            raise ValueError("docx 中缺少 word/document.xml")

        # 4. 读取原始文本（用于校对）
        tree = ET.parse(doc_xml_path)
        root = tree.getroot()

        # 定义命名空间（Word 文档的默认命名空间）
        ns = {'w': 'http://schemas.openxmlformats.org/wordprocessingml/2006/main'}
        ET.register_namespace('', 'http://schemas.openxmlformats.org/wordprocessingml/2006/main')

        # 提取所有 w:t 元素中的文本
        text_elements = root.findall('.//w:t', ns)
        full_text = "\n".join([elem.text for elem in text_elements if elem.text])

        # 5. 调用大模型获取校对建议（复用之前逻辑）
        suggestions, prompt, content = get_polish_suggestions(full_text)  # 需要实现此函数（见下文）

        # 6. 应用修改：遍历 text_elements，根据 suggestions 替换文本
        # 这里简化处理：将文本元素内容替换
        # 注意：suggestions 中需要提供段落索引和原文本，由于 XML 结构复杂，我们按顺序匹配文本元素
        # 简单实现：将所有文本合并后，按句子替换（可能不精确，但作为演示）
        # 更精确的做法：逐元素匹配替换
        # 我们采用简单但实用的方法：整体替换
        if suggestions.get("proofreading"):
            # 构建一个映射：原文本 -> 修正文本
            replacements = {item["original"]: item["corrected"] for item in suggestions["proofreading"]}
            # 遍历所有 w:t 元素
            for elem in text_elements:
                if elem.text and elem.text in replacements:
                    elem.text = replacements[elem.text]

        # 7. 保存修改后的 XML
        tree.write(doc_xml_path, encoding='utf-8', xml_declaration=True)

        # 8. 重新打包为 docx
        output_dir = Path(docx_path).parent
        polished_name = Path(docx_path).stem + "_polished.docx"
        polished_path = output_dir / polished_name
        with zipfile.ZipFile(polished_path, 'w') as zip_out:
            for root_dir, _, files in os.walk(tmpdir):
                for file in files:
                    full_path = os.path.join(root_dir, file)
                    arcname = os.path.relpath(full_path, tmpdir)
                    zip_out.write(full_path, arcname)

        return str(polished_path), suggestions, prompt, content, full_text


def get_polish_suggestions(full_text: str):
    import requests
    import json
    import re

    prompt = f"""
    你是一个专业的文档校对与优化助手。请仔细阅读以下文档内容，并找出所有可能的拼写错误、语法问题以及表达不流畅的句子。

    **文档内容：**
    {full_text}

    **任务：**
    1. 找出文档中所有拼写错误或语法错误，每条记录类型为 "spelling"，并给出原文（original）和建议修改后的正确写法（corrected）。
    2. 找出所有表达不流畅或用词不当的句子，进行润色，每条记录类型为 "improvement"，给出原句（original）和优化后的句子（corrected）。
    3. 如果没有任何需要修改的地方，返回空列表 []。
    4. 请尽可能多地列出修改建议，不要遗漏任何可改进之处。

    **输出格式（必须严格遵守 JSON，不要添加任何其他内容）：**
    {{
      "proofreading": [
        {{"type": "spelling", "original": "原文中的错误词", "corrected": "建议的正确词"}},
        {{"type": "improvement", "original": "需要润色的原句", "corrected": "优化后的句子"}}
      ]
    }}
    """
    print(prompt)
    payload = {
        "model": "qwen2.5:1.5b",
        "prompt": prompt,
        "stream": False,
        "options": {"temperature": 0.2}
    }
    response = requests.post("http://localhost:11434/api/generate", json=payload, timeout=300)
    response.raise_for_status()
    content = response.json()["response"]
    print(content)
    json_match = re.search(r'\{.*\}', content, re.DOTALL)
    if json_match:
        return json.loads(json_match.group()), prompt, content
    else:
        return {"proofreading": []}, prompt, content
