project('harmony', 'cpp', default_options : ['warning_level=3', 'cpp_debugstl=true'], meson_version : '>=0.50.0')

#コンパイルオプション調整
cppcompiler = meson.get_compiler('cpp').get_argument_syntax()

if cppcompiler == 'msvc'
    options = ['/std:c++latest', '/source-charset:utf-8', '/Zc:__cplusplus']
elif cppcompiler == 'gcc'
    options = ['-std=c++2a']
endif

#VSプロジェクトに編集しうるファイルを追加する
vs_files = ['include/harmony.hpp', 'test/harmony_test.cpp']

#インクルードディレクトリ
include_dir = include_directories('include')

#依存プロジェクト
boostut_proj = subproject('boost.ut')
boostut_dep = boostut_proj.get_variable('boostut_dep')
tlexpected_proj = subproject('expected')
tlexpected_dep = tlexpected_proj.get_variable('tlexpected_dep')

exe = executable('harmony_test', 'test/harmony_test.cpp', include_directories : include_dir, extra_files : vs_files, cpp_args : options, dependencies : [boostut_dep, tlexpected_dep])

#テストの設定
test('harmony test', exe)