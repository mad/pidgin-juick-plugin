<meta http-equiv="content-type" content="text/html; charset=utf-8" />

[![www](http://img171.imageshack.us/img171/946/juick.th.png)](http://img171.imageshack.us/img171/946/juick.png)
[![www](http://img38.imageshack.us/img38/3261/jubonologinru.th.png)](http://img38.imageshack.us/img38/3261/jubonologinru.png)
[![www](http://img169.imageshack.us/img169/830/pidginrightclick.th.png)](http://img169.imageshack.us/img169/830/pidginrightclick.png)

# Установка

Установить плагин можно одним из способов:

- Скачать нужную версию плагина ([downloads](http://github.com/mad/pidgin-juick-plugin/downloads)):

        wget http://github.com/mad/pidgin-juick-plugin/downloads/juick.so
        cp juick.so ~/.purple/plugins/

    пользователи Windows, копируйте в папку `%APPDATA%\\.purple\plugins\`

- Собрать из исходников (потребуется dev хидеры libpurple и pidgin):

        git clone git://github.com/mad/pidgin-juick-plugin.git
        cd pidgin-juick-plugin
        make
        cp juick.so ~/.purple/plugins/

Для активации плагина откройте окно пиджина, и нажмите Ctrl+U, появится окно
со списком плагинов - напротив Juick 0.2 поставьте галочку.

# Отличия от плагина mad (v0.1)

 1. Жуйктеги проставляются для длинных сообщений, в которых общее число тегов > 100;
 2. Проставляется реальное время каждого сообщения;
 3. Работает с ботом jubo@nologin.ru;
 4. UTF8 для внутренней обработки сообщений, в результате, правильно работает на windows;

Тестировался плагин на pidgin 2.6, но должен работать на pidgin 2.5, если это
не так, то сообщайте [сюда](http://juick.com/pktfag) или [сюда](http://juick.com/mad).

Конфликтует с плагином convcolors (Conversation Colors - Цвета беседы). Поэтому для работы плагина Juick, надо отключать плагин convcolors.

# TODO
 1. На русский язык перевести;
 2. Добавить горячие клавиши;
 3. Добавить аватары;
 4. Добавить загрузку превью картинок;
