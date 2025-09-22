#!/usr/bin/env python3
import json
import xml.etree.ElementTree as ET
from pathlib import Path
import sys

def analyze_ts_file(ts_file_path):
    try:
        tree = ET.parse(ts_file_path)
        root = tree.getroot()
       
        total_messages = 0
        unfinished_messages = 0
       
        for message in root.findall('.//message'):
            translation = message.find('translation')
            if translation is None:
                continue
               
            translation_type = translation.get('type', '').lower()
           
            if translation_type == 'vanished':
                continue
               
            total_messages += 1
           
            if translation_type == 'unfinished':
                unfinished_messages += 1
                   
        finished_messages = total_messages - unfinished_messages
       
        if total_messages == 0:
            percentage = 0
        else:
            percentage = int((finished_messages / total_messages) * 100)
           
        return {
            'total_messages': total_messages,
            'translated_messages': finished_messages,
            'percentage': percentage
        }
       
    except ET.ParseError as e:
        print(f"Error parsing {ts_file_path}: {e}")
        return None
    except Exception as e:
        print(f"Error processing {ts_file_path}: {e}")
        return None

def extract_language_code(filename):
    if filename.startswith('QontrolPanel_') and filename.endswith('.ts'):
        return filename[len('QontrolPanel_'):-len('.ts')]
   
    return filename

def main():
    script_dir = Path(__file__).parent
    i18n_dir = script_dir.parent / 'i18n'
    compiled_dir = i18n_dir / 'compiled'
   
    compiled_dir.mkdir(exist_ok=True)
   
    output_file = compiled_dir / 'translation_progress.json'
   
    print("Analyzing translation files...")
    print(f"Looking for .ts files in: {i18n_dir}")
   
    ts_files = list(i18n_dir.glob('*.ts'))
   
    if not ts_files:
        print("No .ts files found in i18n directory!")
        sys.exit(1)
   
    print(f"Found {len(ts_files)} translation files")
   
    translation_progress = {}
   
    for ts_file in ts_files:
        print(f"Analyzing {ts_file.name}...")
       
        language_code = extract_language_code(ts_file.name)
        
        # Special case for English - always 100%
        if language_code.lower() in ['en', 'english', 'en_us', 'en_gb']:
            translation_progress[language_code] = 100
            print(f"  {language_code}: 100% (English - always complete)")
            continue
       
        stats = analyze_ts_file(ts_file)
        if stats is None:
            print(f"Skipping {ts_file.name} due to errors")
            continue
       
        translation_progress[language_code] = stats['percentage']
       
        print(f"  {language_code}: {stats['translated_messages']}/{stats['total_messages']} = {stats['percentage']}%")
   
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(translation_progress, f, indent=2, ensure_ascii=False)
       
        print(f"\nTranslation progress saved to: {output_file}")
        print("Contents:")
        print(json.dumps(translation_progress, indent=2))
       
    except Exception as e:
        print(f"Error saving JSON file: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()