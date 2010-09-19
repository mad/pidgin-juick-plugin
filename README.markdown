<meta http-equiv="content-type" content="text/html; charset=utf-8" />

[![www](http://img171.imageshack.us/img171/946/juick.th.png)](http://img171.imageshack.us/img171/946/juick.png)
[![www](http://img38.imageshack.us/img38/3261/jubonologinru.th.png)](http://img38.imageshack.us/img38/3261/jubonologinru.png)
[![www](http://img7.imageshack.us/img7/830/pidginrightclick.th.png)](http://img7.imageshack.us/img7/830/pidginrightclick.png)

# FAQ

- Почему не показывает жуйкменю правая кнопка мыши?

  Правая кнопка мыши работает только в пиджин 2.6, в пиджин 2.4, 2.5 ее невозможно встроить из-за технических ограничений.

- Почему жуйкссылки в моем уютном бложике преобразуются к виду: <j://q?account=someuser@gmail.com/Home&reply=#&body=#352047/7>?

  Это не баг с форматированием. Если вы копируете вручную из окошка адрес (типа #352047/7), а не пользуетесь правой кнопкой Insert, то в пиджин есть пункт "Вставить как простой текст", или, если уже скопировали, то нажимайте кнопку "Восстановить форматирование". Так любую ссылку, вставленную в окно ввода, пиджин преобразует.

- Та же самая некрасивость (<j://q?account=someuser@gmail.com/Home&reply=#&body=#352047/7>), но почему-то все жуйкссылки так выглядят в окне пиджина и не посвечиваются.

  Вероятно, конфликт с каким-то плагином. Точно известно, что плагин convcolors (Conversation Colors - Цвета беседы) не дружит с жуйкплагином. Поэтому для работы плагина Juick, надо отключать плагин convcolors.

- Почему у меня неправильно выделяет жирным, начиная с первого жуйктега?

  У Вас аккаунт зарегестрирован на gmail.com и у Вас пиджин 2.4 или 2.5. Это не ошибка плагина, а ошибка пиджина, плагин вообще не выделяет ничего, ни жирным, ни курсивом, ни подчеркиванием. В пиджине 2.6 мы исправили этот баг, а в пиджине 2.4 или 2.5 даже это исправление не работает.

# Установка

Установить плагин можно одним из способов:

Странности с пакетами нужны только для установки перевода на русский. Если перевод вам не нужен, то версия плагина остается такой же, что и раньше, то есть 0.2.9.

- Debian
        wget http://github.com/mad/pidgin-juick-plugin/downloads/pidgin-juick-plugin.deb
        dpkg -i pidgin-juick-plugin.deb

        или подключить репозиторий @librarian:

                Добавляем ключ:
                        gpg --keyserver keyserver.ubuntu.com --recv-key 4A03040C
                        gpg --export 4A03040C | sudo apt-key add -
                Добавляем репозиторий
                        echo "deb http://deb.libc6.org/ lenny main" | sudo tee -a /etc/apt/sources.list
                Обновляем список пакетов:
                        sudo aptitude update
                Устанавливаем:
                        sudo aptitude install pidgin-juick-plugin
- ArchLinux
        yaourt pidgin-juick-plugin
        или взять [PKGBUILD](http://github.com/mad/pidgin-juick-plugin/blob/master/packaging/archlinux/PKGBUILD)
- Windows
        wget http://github.com/mad/pidgin-juick-plugin/downloads/pidgin-juick-plugin.exe
        pidgin-juick-plugin.exe

- Скачать нужную версию плагина ([downloads](http://github.com/mad/pidgin-juick-plugin/downloads))

- Собрать из исходников (потребуется dev хидеры libpurple и pidgin):

        Загрузка исходников:
                wget http://github.com/mad/pidgin-juick-plugin/downloads/pidgin-juick-plugin-0.3.2.tar.bz2
                tar xjvf pidgin-juick-plugin-0.3.2.tar.bz2
        или
                git clone git://github.com/mad/pidgin-juick-plugin.git

        cd pidgin-juick-plugin
        ./waf configure --prefix=/usr
        sudo ./waf install

Для активации плагина откройте окно пиджина, и нажмите Ctrl+U, появится окно
со списком плагинов - напротив Juick 0.3/Жуйк 0.3 поставьте галочку.

# Новое в жуйкплагине 0.3
 1. Переход на систему сборки waf
 2. Перевод на русский язык

# Новое в жуйкплагине 0.2.9
 1. Исправлено зависание пиджина, если делаешь рекомендацию (! #1234)
 2. Добавлена работа с пиджин 2.5, 2.4
 3. Добавлена опция 'Show Juick conversation when click on juick tag in other conversation'
 4. Добавлена опция 'Insert when left click, don't show'
 5. Исправлен баг при обработке juick-команды HELP
 6. Исправлено зависание пиджина, когда получаешь PM-сообщние
 7. Исправлен баг, связанный со скрытием новых бесед
 8. Добавлены команды в жуйкменю: subscribe, unsubscribe, see webpage, recommend post
 9. Исправлен доступ из других контактов
 10. Исправлено зависание пиджина, когда показывается предупреждающее сообщение
 11. Добавлено reply to ко всем ответам

Новости к предыдущим версиям плагина находятся в файле ([ChangeLog](http://github.com/mad/pidgin-juick-plugin/blob/master/ChangeLog)).

# TODO
 1. Добавить горячие клавиши;
 2. Добавить аватары;
 3. Добавить загрузку превью картинок;
