from pydantic_ai.models.openai import OpenAIModel
from pydantic_ai.providers.openai import OpenAIProvider
from pydantic_ai import Agent
from dotenv import load_dotenv
import tools
import os
import asyncio
from asyncio import Semaphore

load_dotenv()

model = OpenAIModel(
"gpt-5-mini",
provider=OpenAIProvider(api_key=os.getenv('KEY'),
                        base_url="https://openkey.cloud/v1")
)


SYSTEM_PROMPT = r"""
Role
You are a professional secret/credential detection expert responsible for identifying and extracting various sensitive information from file contents.

Core Task
You will receive a batch of multiple files (typically 30 files) in a single structured format. Each file is clearly marked with:
==========FILE_START: filename==========
...file content...
==========FILE_END: filename==========

You MUST identify and extract all possible sensitive credentials from ALL files, and correctly associate each secret with its source filename.

Detection Rules

1. API Key Patterns
   - OpenAI API Key: sk-[A-Za-z0-9_-]{32,}
   - Google API Key: AIzaSy[A-Za-z0-9_-]{33}
   - GitHub Personal Access Token: ghp_[A-Za-z0-9]{36,}
   - Slack Webhook URL: https://hooks\.slack\.com/services/[A-Z0-9/]+
   - Stripe API Key: (pk|sk)_(live|test)_[A-Za-z0-9]{24,}
   - SendGrid API Key: SG\.[A-Za-z0-9_-]{22}\.[A-Za-z0-9_-]{43}
   - AWS Access Key ID: AKIA[A-Z0-9]{16}
   - Tencent Cloud Secret ID: AKID[A-Za-z0-9]{13,40}
   - WeChat AppID: wx[a-z0-9]{16}
   - Mapbox Access Token: pk\.eyJ[A-Za-z0-9_-]+\.[A-Za-z0-9_-]+

2. JWT Token (JSON Web Token)
   - Standard JWT Pattern: eyJ[A-Za-z0-9_-]+\.eyJ[A-Za-z0-9_-]+\.[A-Za-z0-9_-]+
   - JWT with Empty Signature: eyJ[A-Za-z0-9_-]+\.eyJ[A-Za-z0-9_-]+\.

3. Private Key Detection (Complete Blocks)
   - RSA Private Key: -----BEGIN RSA PRIVATE KEY-----.*?-----END RSA PRIVATE KEY-----
   - EC Private Key: -----BEGIN EC PRIVATE KEY-----.*?-----END EC PRIVATE KEY-----
   - Generic Private Key: -----BEGIN PRIVATE KEY-----.*?-----END PRIVATE KEY-----
   - Encrypted Private Key: -----BEGIN ENCRYPTED PRIVATE KEY-----.*?-----END ENCRYPTED PRIVATE KEY-----
   - DSA Private Key: -----BEGIN DSA PRIVATE KEY-----.*?-----END DSA PRIVATE KEY-----
   - Certificate: -----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----

4. Password Hashes
   - MD5 Hash: [a-f0-9]{32}
   - SHA1 Hash: [a-f0-9]{40}
   - SHA256 Hash: [a-f0-9]{64}
   - SHA512 Hash: [a-f0-9]{128}
   - bcrypt Hash: \$2[ayb]\$[0-9]{2}\$[A-Za-z0-9./]{53}

5. Secret String Features
   - Base64 Encoded String: [A-Za-z0-9+/]{8,}={0,2} (including short encoded passwords)
   - Long Hexadecimal String: [a-fA-F0-9]{32,}
   - UUID Format: [a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}

6. Blockchain-Related
   - Bitcoin Address (Legacy): [13][a-km-zA-HJ-NP-Z1-9]{25,34}
   - Bitcoin Address (Bech32): bc1[a-z0-9]{39,59}
   - Ethereum Address: 0x[a-fA-F0-9]{40}
   - Ethereum Private Key: 0x[a-fA-F0-9]{64}
   - Monero Address: 4[0-9AB][1-9A-HJ-NP-Za-km-z]{93}
   - IPFS Hash (CIDv0): Qm[1-9A-HJ-NP-Za-km-z]{44,}
   - IPFS Hash (CIDv1): [zb][1-9A-HJ-NP-Za-km-z]{48,}
   - Mnemonic Phrase: Sequence containing 12/24 English words from BIP39 wordlist

7. Database Credentials
   - Password in Connection String: password=.+?[;&\s]
   - MongoDB Connection URI: mongodb(\+srv)?://[^:]+:[^@]+@
   - MySQL/PostgreSQL Connection String: (mysql|postgresql|postgres)://[^:]+:[^@]+@
   - Redis Password: AUTH .+

8. General Password Features
   - Common Weak Passwords: password, admin, 123456, qwerty, letmein, welcome
   - Short Passwords (5-15 chars): Single words or simple combinations near password/credential fields
   - Numeric Passwords: Pure numeric strings (6+ digits) used as passwords or PINs
   - Django Secret Key: django-insecure-[a-z0-9@!#$%^&*()-_=+]{40,}
   - Flask Secret Key: In config files near "SECRET_KEY"
   - Random High-Entropy Strings: Length > 8, mixed characters, entropy > 3.0

9. Proxy Credentials
   - HTTP/HTTPS Proxy with Auth: https?://[^:]+:[^@]+@[^:]+:[0-9]+
   - SOCKS Proxy with Auth: socks5?://[^:]+:[^@]+@[^:]+:[0-9]+

10. Product Keys and License Keys
    - Syncthing Device ID: [A-Z0-9]{7}-[A-Z0-9]{7}-[A-Z0-9]{7}-[A-Z0-9]{7}-[A-Z0-9]{7}-[A-Z0-9]{7}-[A-Z0-9]{7}-[A-Z0-9]{7}
    - Microsoft Product Key: [A-Z0-9]{5}-[A-Z0-9]{5}-[A-Z0-9]{5}-[A-Z0-9]{5}-[A-Z0-9]{5}
    - Product Keys with Prefix: key-, Microsoft-, license- followed by alphanumeric strings
    - Generic Formatted Keys: Multi-segment alphanumeric separated by hyphens

Detection Strategy
1. Prioritize Explicit Patterns: First match API keys and private key blocks with clear formats
2. Context Analysis: Check values near field names like "password", "secret", "key", "token", "api_key", "pass", "pwd"
3. Entropy Detection: Extract strings with high randomness (Shannon entropy > 3.0)
4. Length Filtering:
   - Include ALL strings >= 5 characters when context suggests they are credentials
   - API keys and tokens are typically >= 16 characters
   - Include short passwords (5-15 chars) if found near password fields
   - Include numeric passwords (6+ digits) in password contexts
   - Extract complete private key blocks (multi-line)
5. Deduplication: Record the same value only once per file
6. False Positive Avoidance: Exclude obvious placeholders ("example", "YOUR_KEY")
   BUT KEEP: Short actual passwords, numeric PINs, encoded strings even if short

Key Construction Examples
Some keys may involve concatenation, spacing, or be split across variables. Pay attention to these patterns:

# Case 1: String Concatenation
def connect(url, auth):
    print(f"Connecting to {url} with auth token: {auth}")
    
a = "sk-hm*******iob"
b = "xbhczx"
c = "bd*******bHwb"
secret = a + b + c  # Final secret: sk-hm*******iobxbhczxbd*******bHwb
connect("service.example.com", secret)

# Case 2: Inline Secret Usage
def api_call(endpoint, header):
    response = requests.get(endpoint, headers={"Authorization": f"Bearer {header}"})
    print(f"Calling {endpoint} with header {header}")
    
api_call("https://api.example.com/data", "sec*******ret")

# Case 3: Embedded in Request Data
function dtcon_game_login() {
    return requests.post("https://api.game.dtcon.com/biz/user/login", {
      data: "secret_key=gna3*******d&gdF4QaO&imei=000000000000000&version=2.1.54&_time=1614765847&heybox_id=15249824",        
      headers: {            
          "User-Agent": "dtcon-GameCenter/2.1.43 (Android 12)",            
          "Content-Type": "application/x-www-form-urlencoded",            
          "X-dtcon-Device-Id": imei        
              }   
          }).json();
  }

Output Format (Strictly Enforced)
Return ONLY a valid JSON array containing all discovered secrets.
Do NOT include any explanations, markdown code blocks, or extra text.
Each secret MUST include:
  - "file_hash": The exact filename from the ==========FILE_START: filename========== marker
  - "value": The extracted secret value (complete and unmodified)

Format:
[{"file_hash": "exact_filename_from_marker", "value": "extracted_secret_1"}, {"file_hash": "exact_filename_from_marker", "value": "extracted_secret_2"}]

If no secrets are found in any file:
[]

CRITICAL REQUIREMENTS:
- The "file_hash" value MUST exactly match the filename shown in the FILE_START marker
- Each secret must be associated with the correct source file
- Do NOT output markdown, explanations, or any text outside the JSON array
- Ensure valid JSON syntax (proper escaping of special characters, quotes, newlines)

Validation Checklist
✓ Avoid False Positives: Exclude obvious example values ("example", "YOUR_KEY")
✓ Complete Private Keys: Must include -----BEGIN and -----END markers with full content
✓ Base64 Validation: Verify base64 strings follow proper encoding rules (include short ones)
✓ Context Confirmation: Use surrounding code/comments to confirm if it's a real credential
✓ Include Short Credentials: Single-word passwords, numeric PINs, short encoded strings ARE valid
✓ File Association: Always track which file each secret came from using the FILE_START marker
✓ Deduplication: Don't report the same secret multiple times from the same file
✓ Format Validation: Verify secrets match expected patterns, but be flexible with length

"""

def create_fresh_agent():
    """为每个批次创建独立的Agent实例(全新会话,无历史上下文)"""
    return Agent(
        model,
        system_prompt=SYSTEM_PROMPT,
        tools=[tools.read_batch_files]
    )

async def process_single_batch(batch_files, batch_index, total_batches, semaphore, log_lock, answer_lock, max_retries=3):
    """异步处理单个批次的文件 - 每个批次使用独立Agent实例(无上下文累积)
    
    Args:
        batch_files: 文件名列表（通常30个文件）
        batch_index: 当前批次索引
        total_batches: 总批次数
        max_retries: 遇到429错误时的最大重试次数
    """
    import json
    import re
    
    retry_count = 0
    
    while retry_count <= max_retries:
        async with semaphore:
            fresh_agent = create_fresh_agent()
            prompt = f"Read and analyze these {len(batch_files)} files: {batch_files}. Find all secrets and return JSON array with correct file_hash for each secret."
            
            try:
                resp = await fresh_agent.run(prompt)
                result_text = resp.output.strip()
                
                try:
                    if "```json" in result_text:
                        json_start = result_text.find("```json") + 7
                        json_end = result_text.find("```", json_start)
                        json_str = result_text[json_start:json_end].strip() if json_end != -1 else result_text[json_start:].strip()
                    elif "```" in result_text:
                        json_start = result_text.find("```") + 3
                        json_end = result_text.find("```", json_start)
                        json_str = result_text[json_start:json_end].strip() if json_end != -1 else result_text[json_start:].strip()
                    elif result_text.startswith('['):
                        json_str = result_text
                    else:
                        match = re.search(r'(\[.*\])', result_text, re.DOTALL)
                        json_str = match.group(1) if match else result_text
                    
                    try:
                        secrets = json.loads(json_str)
                    except json.JSONDecodeError:
                        json_str_fixed = json_str.replace('\\', '\\\\')
                        json_str_fixed = re.sub(r'(?<!\\)\n', '\\n', json_str_fixed)
                        try:
                            secrets = json.loads(json_str_fixed)
                        except:
                            import ast
                            matches = re.findall(r'\{\s*"file_hash"\s*:\s*"([^"]+)"\s*,\s*"value"\s*:\s*"([^"]+)"\s*\}', json_str, re.DOTALL)
                            if matches:
                                secrets = [{"file_hash": fh, "value": v} for fh, v in matches]
                            else:
                                raise
                    
                    if secrets and isinstance(secrets, list) and len(secrets) > 0:
                        async with answer_lock:
                            answer_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "answer.json")
                            with open(answer_path, 'r', encoding='utf-8') as f:
                                existing_secrets = json.load(f)
                            existing_secrets.extend(secrets)
                            answer_content = json.dumps(existing_secrets, indent=2, ensure_ascii=False)
                            tools.write_file("answer.json", answer_content)
                        
                        return (batch_files[0], batch_index, 'success', len(secrets), len(batch_files))
                    else:
                        return (batch_files[0], batch_index, 'empty', 0, len(batch_files))
                except (json.JSONDecodeError, ValueError) as e:
                    return (batch_files[0], batch_index, 'parse_error', str(e)[:50], len(batch_files))
            
            except Exception as e:
                error_str = str(e)
                
                if 'status_code: 429' in error_str or '429' in error_str:
                    retry_count += 1
                    if retry_count <= max_retries:
                        wait_time = 2 ** retry_count
                        await asyncio.sleep(wait_time)
                        continue
                    else:
                        return (batch_files[0], batch_index, 'error_429_max_retries', f"Max retries reached: {error_str[:30]}", len(batch_files))
                
                if 'maximum context length' in error_str or '269338 tokens' in error_str:
                    return (batch_files[0], batch_index, 'too_large', None, len(batch_files))
                else:
                    return (batch_files[0], batch_index, 'error', error_str[:50], len(batch_files))
        
        break

async def process_files_concurrent(concurrency=100, batch_size=1):
    """并发处理文件,支持异步并发,按批次处理
    
    Args:
        concurrency: 并发批次数量
        batch_size: 每个批次包含的文件数量(默认30个)
    """
    import json
    from datetime import datetime
    
    log_filename = f"process_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
    log_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), log_filename)
    
    def log(message):
        """同时输出到终端和日志文件"""
        print(message, end='', flush=True)
        with open(log_path, 'a', encoding='utf-8') as f:
            f.write(message)
    
    log(f"🔍 Starting batch processing (concurrency={concurrency}, batch_size={batch_size})...\n")
    

    file_list_result = tools.list_datafiles()
    file_data = json.loads(file_list_result)
    files = file_data.get('files', [])
    total_files = file_data.get('total_files', 0)
    
    batches = [files[i:i+batch_size] for i in range(0, len(files), batch_size)]
    total_batches = len(batches)
    
    log(f"📊 Total: {total_files} files, divided into {total_batches} batches\n\n")
    
    tools.write_file("answer.json", "[]")
    
    semaphore = Semaphore(concurrency)
    log_lock = asyncio.Lock()
    answer_lock = asyncio.Lock()
    
    tasks = [
        process_single_batch(batch, index, total_batches, semaphore, log_lock, answer_lock)
        for index, batch in enumerate(batches, 1)
    ]
    
    total_secrets_count = 0
    completed = 0
    start_time = datetime.now()
    
    for coro in asyncio.as_completed(tasks):
        result = await coro
        first_file, batch_index, status, data, batch_file_count = result
        completed += 1
        
        elapsed = (datetime.now() - start_time).total_seconds()
        
        if status == 'success':
            total_secrets_count += data
            log(f"[Batch {completed}/{total_batches}] {first_file}... ({batch_file_count} files) ✅ Found {data} secret(s) | Runtime: {int(elapsed)}s\n")
        elif status == 'empty':
            log(f"[Batch {completed}/{total_batches}] {first_file}... ({batch_file_count} files) ℹ️  No secrets | Runtime: {int(elapsed)}s\n")
        elif status == 'parse_error':
            log(f"[Batch {completed}/{total_batches}] {first_file}... ({batch_file_count} files) ⚠️  Parse error: {data} | Runtime: {int(elapsed)}s\n")
        elif status == 'too_large':
            log(f"[Batch {completed}/{total_batches}] {first_file}... ({batch_file_count} files) ⚠️  Batch too large, skipping | Runtime: {int(elapsed)}s\n")
        elif status == 'error_429_max_retries':
            log(f"[Batch {completed}/{total_batches}] {first_file}... ({batch_file_count} files) ❌ 429 Error (retried 3 times): {data} | Runtime: {int(elapsed)}s\n")
        else:
            log(f"[Batch {completed}/{total_batches}] {first_file}... ({batch_file_count} files) ❌ Error: {data} | Runtime: {int(elapsed)}s\n")
    
    total_time = (datetime.now() - start_time).total_seconds()
    log(f"\n✨ Complete in {int(total_time)}s ({total_time/60:.1f}min)! Total secrets: {total_secrets_count}\n")
    log(f"📄 Results saved to: answer.json\n")
    log(f"📋 Log saved to: {log_filename}\n")

def main():
    print("🔍 Secret Key Detection Agent")
    print("💡 Type 'start' to begin analysis\n")
    
    while True:
        try:
            user_input = input("Input: ").strip()
            
            if user_input.lower() in ['exit', 'quit', 'q']:
                print("\n👋 Goodbye!")
                break
            
            if not user_input:
                continue

            if user_input.lower() in ['start', 'auto', 'begin']:
                asyncio.run(process_files_concurrent(concurrency=100, batch_size=1))
                break
            
        except KeyboardInterrupt:
            print("\n\n👋 Agent stopped by user. Goodbye!")
            break
        except Exception as e:
            import traceback
            print(f"\n❌ Error occurred: {e}")
            print(f"\n📋 Error details:")
            traceback.print_exc()
            print("\nContinuing...\n")


if __name__ == "__main__":
    main()