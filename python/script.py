import sys
import os.path
import re
import htmlmin
import csscompressor

def main(input_file):

    # Internal CSS compressed string
    def __css_compress(match):
        css = match.group(1)
        compressed_css = csscompressor.compress(css)
        return f"<style>{compressed_css}</style>"

    with open(input_file, 'r', encoding='utf-8') as f:
        html_with_minified_css = re.sub(r'<style>(.*?)</style>', __css_compress, f.read(), flags=re.DOTALL)

    # HTML now ready to be minified
        minified_html = htmlmin.minify(html_with_minified_css,
                                       remove_comments=True,
                                       remove_empty_space=True,
                                       reduce_boolean_attributes=True
                                       )
    with open(input_file[:-5]+'_minified.html', 'w', encoding='utf-8') as f:
        f.write(minified_html)

if __name__ == '__main__':
    if len(sys.argv) == 2 and sys.argv[1][-5:] == '.html' and os.path.isfile(sys.argv[1]):
        main(sys.argv[1])