import os
import subprocess
import time
import shutil
import zipfile
from flask import Flask, request, send_file, jsonify
from werkzeug.utils import secure_filename

# ========== 核心配置（仅改BASE_DIR即可） ==========
app = Flask(__name__, static_folder='../', template_folder='../')
# 基础路径（总文件夹路径，改成你的！）
BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '../'))
UPLOAD_FOLDER = os.path.join(BASE_DIR, 'data/uploads')  # 上传根目录
OUTPUT_FOLDER = os.path.join(BASE_DIR, 'data/outputs')  # 输出目录
CPP_EXE_PATH = os.path.join(os.path.dirname(__file__), 'Project1.exe')  # C++ exe路径
ALLOWED_EXTENSIONS = {
    'tex': 'tex',  # TEX文件
    'bib': 'bib',  # BIB引用文件
    'cls': 'cls',  # CLS格式文件
    'zip': 'zip'  # 图片压缩包（ZIP）
}

# 创建目录
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
os.makedirs(OUTPUT_FOLDER, exist_ok=True)


# 检查文件类型是否合法
def allowed_file(filename, file_type):
    ext = filename.rsplit('.', 1)[1].lower() if '.' in filename else ''
    return ext == ALLOWED_EXTENSIONS.get(file_type, '')


# 解压ZIP文件到指定目录
def unzip_file(zip_path, target_dir):
    try:
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(target_dir)
        return True
    except Exception as e:
        print(f"解压ZIP失败：{e}")
        return False


# 清理过期/临时文件（每天清理7天前的文件）
def clean_expired_files():
    now = time.time()
    # 清理上传目录
    for folder in [UPLOAD_FOLDER, OUTPUT_FOLDER]:
        for item in os.listdir(folder):
            item_path = os.path.join(folder, item)
            # 清理7天前的文件/空文件夹
            if os.path.isfile(item_path) and now - os.path.getmtime(item_path) > 7 * 24 * 3600:
                os.remove(item_path)
                print(f"清理过期文件：{item_path}")
            elif os.path.isdir(item_path) and now - os.path.getmtime(item_path) > 24 * 3600:
                shutil.rmtree(item_path)
                print(f"清理临时文件夹：{item_path}")


# 接口1：多文件上传+转换
@app.route('/upload', methods=['POST'])
def upload_file():
    clean_expired_files()

    # 1. 接收所有上传的文件（TEX/BIB/CLS/图片ZIP）
    tex_file = request.files.get('tex_file')
    bib_file = request.files.get('bib_file')
    cls_file = request.files.get('cls_file')
    img_zip_file = request.files.get('img_zip_file')

    # 2. 校验必填项（TEX文件必须有）
    # if not tex_file or tex_file.filename == '':
    #     return jsonify({'code': 400, 'msg': '请上传TEX文件！'}), 400
    # if not allowed_file(tex_file.filename, 'tex'):
    #     return jsonify({'code': 400, 'msg': 'TEX文件格式错误（仅支持.tex）！'}), 400
    if tex_file and tex_file.filename != '' and not allowed_file(tex_file.filename, 'tex'):
        return jsonify({'code': 400, 'msg': 'TEX文件格式错误（仅支持.tex）！'}), 400

    # 3. 校验可选文件（BIB/CLS/图片ZIP）
    if bib_file and bib_file.filename != '' and not allowed_file(bib_file.filename, 'bib'):
        return jsonify({'code': 400, 'msg': 'BIB文件格式错误（仅支持.bib）！'}), 400
    if cls_file and cls_file.filename != '' and not allowed_file(cls_file.filename, 'cls'):
        return jsonify({'code': 400, 'msg': 'CLS文件格式错误（仅支持.cls）！'}), 400
    if img_zip_file and img_zip_file.filename != '' and not allowed_file(img_zip_file.filename, 'zip'):
        return jsonify({'code': 400, 'msg': '图片压缩包格式错误（仅支持.zip）！'}), 400

    # 4. 创建临时目录（每个用户的上传文件单独存放，避免冲突）
    temp_dir = os.path.join(UPLOAD_FOLDER, f"temp_{int(time.time())}")
    os.makedirs(temp_dir, exist_ok=True)
    # 创建图片解压目录
    img_dir = os.path.join(temp_dir, 'figures')
    os.makedirs(img_dir, exist_ok=True)

    # 5. 保存上传的文件（均为绝对路径）
    # 5.1 保存TEX文件
    # tex_filename = secure_filename(tex_file.filename)
    # tex_path = os.path.join(temp_dir, tex_filename)
    # tex_file.save(tex_path)
    if tex_file and tex_file.filename != '':
        tex_filename = secure_filename(tex_file.filename)
        tex_path = os.path.join(temp_dir, tex_filename)
        tex_file.save(tex_path)
    else:
        # 无TEX文件时，使用默认TEX路径
        tex_path = os.path.join(BASE_DIR, 'data\default\document.tex')
        tex_filename = "document"

    # 5.2 保存BIB文件（可选，若用户没传则用默认路径）
    if bib_file and bib_file.filename != '':
        bib_filename = secure_filename(bib_file.filename)
        bib_path = os.path.join(temp_dir, bib_filename)
        bib_file.save(bib_path)
    else:
        # 若用户没传BIB，用默认绝对路径（可删除，强制用户传）
        bib_path = os.path.join(BASE_DIR, 'data\default\\refs.bib')

    # 5.3 保存CLS文件（可选）
    if cls_file and cls_file.filename != '':
        cls_filename = secure_filename(cls_file.filename)
        cls_path = os.path.join(temp_dir, cls_filename)
        cls_file.save(cls_path)
    else:
        # 若用户没传CLS，用默认绝对路径（可删除，强制用户传）
        cls_path = os.path.join(BASE_DIR, 'data\default\\article.cls')

    # 5.4 处理图片压缩包（可选：用户上传则解压，否则用默认路径）
    if img_zip_file and img_zip_file.filename != '':
        # 保存ZIP文件
        zip_filename = secure_filename(img_zip_file.filename)
        zip_path = os.path.join(temp_dir, zip_filename)
        img_zip_file.save(zip_path)
        # 解压ZIP到图片目录
        if not unzip_file(zip_path, img_dir):
            shutil.rmtree(temp_dir)
            return jsonify({'code': 500, 'msg': '图片压缩包解压失败！'}), 500
        # 解压后的图片目录作为参数
        figure_dir = img_dir + '\\'
    else:
        # 若用户没传图片ZIP，使用默认的figures.zip解压
        default_zip_path = os.path.join(BASE_DIR, 'data\default\\figures.zip')
        # 检查默认ZIP文件是否存在
        if not os.path.exists(default_zip_path):
            shutil.rmtree(temp_dir)
            return jsonify({'code': 500, 'msg': '默认图片压缩包不存在！请检查data/default/figures.zip'}), 500
        # 解压默认ZIP到图片目录
        if not unzip_file(default_zip_path, img_dir):
            shutil.rmtree(temp_dir)
            return jsonify({'code': 500, 'msg': '默认图片压缩包解压失败！'}), 500
        # 解压后的图片目录作为参数
        figure_dir = img_dir + '\\'

    # 6. 输出DOCX路径（避免重名）
    output_docx_name = f"tex2docx_{int(time.time())}_{tex_filename.rsplit('.', 1)[0]}.docx"
    output_docx_path = os.path.join(OUTPUT_FOLDER, output_docx_name)

    try:
        # 7. 调用C++程序（所有参数都是绝对路径！）
        cmd = [
            CPP_EXE_PATH,
            tex_path,  # 参数1：上传的TEX
            bib_path,  # 参数2：上传/默认的BIB
            figure_dir,  # 参数3：上传解压/默认的图片目录
            cls_path,  # 参数4：上传/默认的CLS
            output_docx_path  # 参数5：输出DOCX
        ]
        print(f"执行C++命令：{' '.join(cmd)}")

        # 执行C++程序
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=120
        )

        # 检查执行结果
        if result.returncode != 0:
            error_msg = f"C++转换失败：{result.stderr.strip() or '未知错误'}"
            print(f"错误详情：{error_msg}")
            return jsonify({'code': 500, 'msg': error_msg}), 500

        # 8. 转换成功后清理临时文件
        shutil.rmtree(temp_dir)

        # 9. 返回成功结果
        return jsonify({
            'code': 200,
            'msg': 'TEX转DOCX成功！',
            'data': {
                'download_url': f'/download/{output_docx_name}'
            }
        }), 200

    except subprocess.TimeoutExpired:
        shutil.rmtree(temp_dir)  # 超时清理临时文件
        return jsonify({'code': 500, 'msg': '转换超时（超过120秒）！'}), 500
    except Exception as e:
        shutil.rmtree(temp_dir)  # 异常清理临时文件
        error_msg = f"服务器错误：{str(e)}"
        print(f"服务器错误：{error_msg}")
        return jsonify({'code': 500, 'msg': error_msg}), 500


# 接口2：下载DOCX文件
@app.route('/download/<filename>', methods=['GET'])
def download_file(filename):
    file_path = os.path.join(OUTPUT_FOLDER, filename)
    if not os.path.exists(file_path):
        return jsonify({'code': 404, 'msg': '文件不存在（已过期或未生成）！'}), 404
    return send_file(
        file_path,
        as_attachment=True,
        download_name=filename,
        mimetype='application/vnd.openxmlformats-officedocument.wordprocessingml.document'
    )


# 接口3：首页（支持TEX/BIB/CLS/图片ZIP上传）
@app.route('/')
def index():
    return '''
    <!DOCTYPE html>
    <html lang="zh-CN">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>TEX转DOCX在线工具（完整版）</title>
        <style>
            * {margin: 0; padding: 0; box-sizing: border-box; font-family: "Microsoft YaHei", Arial;}
            body {background: #f5f5f5; padding: 50px 0;}
            .container {max-width: 800px; margin: 0 auto; background: white; padding: 40px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1);}
            h1 {text-align: center; color: #333; margin-bottom: 30px;}
            .upload-group {margin-bottom: 20px;}
            .upload-group label {display: block; margin-bottom: 8px; color: #333; font-size: 16px;}
            .upload-box {border: 2px dashed #0078d7; padding: 30px 20px; text-align: center; border-radius: 8px; cursor: pointer; margin-bottom: 10px;}
            .upload-box:hover {border-color: #0056b3;}
            .upload-box i {font-size: 36px; color: #0078d7; margin-bottom: 8px; display: block;}
            .upload-box p {color: #666; font-size: 14px;}
            .file-input {display: none;}
            .file-name {color: #666; font-size: 14px; text-align: center;}
            .btn {background: #0078d7; color: white; border: none; padding: 12px 30px; border-radius: 6px; font-size: 16px; cursor: pointer; margin-top: 20px; display: block; margin-left: auto; margin-right: auto;}
            .btn:hover {background: #0056b3;}
            #result {text-align: center; font-size: 18px; margin-top: 20px; min-height: 24px;}
            .success {color: #10b981;}
            .error {color: #ef4444;}
            .loading {color: #0078d7;}
            a {color: #0078d7; text-decoration: none;}
            a:hover {text-decoration: underline;}
            .required {color: red;}
        </style>
        <link rel="stylesheet" href="https://cdn.bootcdn.net/ajax/libs/font-awesome/6.4.0/css/all.min.css">
    </head>
    <body>
        <div class="container">
            <h1>TEX转DOCX在线工具</h1>

            <!-- TEX文件上传（必填） -->
            <div class="upload-group">
                <label>TEX文件 <span class="required">*</span></label>
                <div class="upload-box" onclick="document.getElementById('texFileInput').click()">
                    <i class="fa fa-file-text-o"></i>
                    <p>点击上传.tex文件（非必填，默认为示例）</p>
                    <input type="file" id="texFileInput" class="file-input" accept=".tex" onchange="showFileName('texFileInput', 'texFileName')">
                </div>
                <p id="texFileName" class="file-name"></p>
            </div>

            <!-- BIB文件上传（可选） -->
            <div class="upload-group">
                <label>BIB文件（可选）</label>
                <div class="upload-box" onclick="document.getElementById('bibFileInput').click()">
                    <i class="fa fa-file-text-o"></i>
                    <p>点击上传.bib文件（非必填，默认为示例）</p>
                    <input type="file" id="bibFileInput" class="file-input" accept=".bib" onchange="showFileName('bibFileInput', 'bibFileName')">
                </div>
                <p id="bibFileName" class="file-name"></p>
            </div>

            <!-- CLS文件上传（可选） -->
            <div class="upload-group">
                <label>CLS文件（可选）</label>
                <div class="upload-box" onclick="document.getElementById('clsFileInput').click()">
                    <i class="fa fa-file-text-o"></i>
                    <p>点击上传.cls文件（非必填，默认为示例）</p>
                    <input type="file" id="clsFileInput" class="file-input" accept=".cls" onchange="showFileName('clsFileInput', 'clsFileName')">
                </div>
                <p id="clsFileName" class="file-name"></p>
            </div>

            <!-- 图片压缩包上传（可选） -->
            <div class="upload-group">
                <label>图片压缩包（可选）</label>
                <div class="upload-box" onclick="document.getElementById('imgZipFileInput').click()">
                    <i class="fa fa-file-archive-o"></i>
                    <p>点击上传包含图片的.zip文件（非必填，默认为示例）</p>
                    <input type="file" id="imgZipFileInput" class="file-input" accept=".zip" onchange="showFileName('imgZipFileInput', 'imgZipFileName')">
                </div>
                <p id="imgZipFileName" class="file-name"></p>
            </div>

            <button class="btn" onclick="uploadFile()" id="uploadBtn">上传并转换</button>
            <div id="result"></div>
        </div>

        <script>
            // 显示选中的文件名
            function showFileName(inputId, displayId) {
                const input = document.getElementById(inputId);
                const display = document.getElementById(displayId);
                if (input.files.length > 0) {
                    display.textContent = `已选择：${input.files[0].name}`;
                }
            }

            // 上传所有文件
            async function uploadFile() {
                const texFile = document.getElementById('texFileInput').files[0];

                // 显示加载状态
                showResult('<i class="fa fa-spinner fa-spin"></i> 转换中，请稍候...', 'loading');
                document.getElementById('uploadBtn').disabled = true;

                // 构建FormData，包含所有上传的文件
                const formData = new FormData();
                formData.append('tex_file', texFile);

                const bibFile = document.getElementById('bibFileInput').files[0];
                if (bibFile) formData.append('bib_file', bibFile);

                const clsFile = document.getElementById('clsFileInput').files[0];
                if (clsFile) formData.append('cls_file', clsFile);

                const imgZipFile = document.getElementById('imgZipFileInput').files[0];
                if (imgZipFile) formData.append('img_zip_file', imgZipFile);

                try {
                    const response = await fetch('/upload', {
                        method: 'POST',
                        body: formData
                    });
                    const data = await response.json();

                    if (data.code === 200) {
                        showResult(`✅ ${data.msg} <a href="${data.data.download_url}" target="_blank">点击下载DOCX文件</a>`, 'success');
                    } else {
                        showResult(`❌ ${data.msg}`, 'error');
                    }
                } catch (e) {
                    showResult('❌ 网络错误，请检查服务是否运行！', 'error');
                } finally {
                    document.getElementById('uploadBtn').disabled = false;
                    // 清空文件选择框
                    document.getElementById('texFileInput').value = '';
                    document.getElementById('bibFileInput').value = '';
                    document.getElementById('clsFileInput').value = '';
                    document.getElementById('imgZipFileInput').value = '';
                    document.getElementById('texFileName').textContent = '';
                    document.getElementById('bibFileName').textContent = '';
                    document.getElementById('clsFileName').textContent = '';
                    document.getElementById('imgZipFileName').textContent = '';
                }
            }

            // 显示结果提示
            function showResult(msg, type='success') {
                const resultDiv = document.getElementById('result');
                resultDiv.innerHTML = msg;
                resultDiv.className = type;
            }
        </script>
    </body>
    </html>
    '''


# 启动服务
if __name__ == '__main__':
    # 打印路径调试
    print(f"基础路径：{BASE_DIR}")
    print(f"上传目录：{UPLOAD_FOLDER}")
    print(f"输出目录：{OUTPUT_FOLDER}")
    print(f"C++程序：{CPP_EXE_PATH}")

    app.run(
        host='0.0.0.0',
        port=5000,
        debug=True,
        threaded=True
    )