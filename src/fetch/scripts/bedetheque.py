#!/usr/bin/env python
# -*- coding: utf-8 -*-

#Copyright 2010 Raphaël Fischer fischer.tellico@free.fr

#This library is free software; you can redistribute it and/or
#modify it under the terms of the GNU Lesser General Public
#License as published by the Free Software Foundation; either
#version 2.1 of the License, or (at your option) version 3, or any
#later version accepted by the membership of KDE e.V. (or its
#successor approved by the membership of KDE e.V.), which shall
#act as a proxy defined in Section 6 of version 3 of the license.

#This library is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#Lesser General Public License for more details.

#You should have received a copy of the GNU Lesser General Public 
#License along with this library.  If not, see <http://www.gnu.org/licenses/>.


# $Id: bedetheque.py 2009-05-28 raphael $

"""
You are welcome to report any bug to Raphaël Fischer, such as missing or wrong information, when comparing to linked bedetheque.com page.
This script currently provides the following information, corresponding to the bedetheque.com fields :

Serie,  rating, synopsis (of the serie), critic, title (of the album), scénarist, artist (dessinateur), colorist, legal deposit (dépot légal), printing date (achevé d'imprimé), estimated value, publisher (editeur), collection, ISBN, number of pages, various information, front and back cover, exemple page, etc... Multiple values are given.

Tellico data source setup:
- source name: bedetheque.com
- Collection type: comics collection
Update should replace existing data : checked
- Result type: tellico
- Path: /path/to/script/bedetheque.com_v3.py
- Recommanded arguments:
Title (album title or serie) =  -t %1
Author =        -a %1
ISBN =  -isbn %1
Keyword (legal deposit) =       -dl %1
Update =        -t %{title} -a %{artist} -isbn %{isbn} -lk %{lien-bel}

Version 0.5.2: 2010-06-01
* Fix some pattern issues preventing all results but fast research

Version 0.5.1: 2009-06-02
* Fix image downloading in some cases.

Version 0.5: 2009-05-28 (Thanks to Félix Sipma and Jérôme Martin)
* A lot of improvement to take advantage of google cache and of proxies (list automatically updated). This should result in faster execution and avoid the automaticaly bannishment from the site based on IP.
* Much changes in the procedure to comply with the updated bedetheque.com own search feature
* Fixed md5 issue with python 2.6
* Changed some default values so as to get simple spec.
* Added -f option for very fast (but limited) search
* Direct link to the original page is now given as a URL field, instead of embedded in commentary. (So as to ease update.)
* Some data are not fetched any more. (Genre, country, language, special info concerning the edition). If want them back, complain.

Version 0.4: 2009-04-03
* Added the following information : edition originale, tirage de tête, tirage limité, broché, intégrale
* Fix updates of reeditions
* Slight improve in speed, espacially for items in large series
* Optional image limit is now on album number
* Fix v0.3 issue that prevented any result to be found since march 09
* Minor bug fixes

Version 0.3: 2009-01-28
* Added back cover and exemple page images.
* Images are now full-size (use '-i' if you prefer thumbnail)
* Added per-album user's rating
* Fixed issue on missing old critics.
* Added search by ISBN or legal deposit date
* Script now also fetches reeditions (remove '-r' argument if you don't want it)
* Multiple parameters can now be sent directly using the syntax '-x argument' like '-a author'.
See home page or end of file ofr further explanation
* '-s' argument is deprecated as bedetheque.com search does not differentiate album title from serie name

Version 0.2: 2008-12-18
* a few patterns have been corrected
* use of the external site cross-search instead of "hand-made" one => execution faster.

Version 0.1 based on Mathias Monnerville allocine.fr script
* Initial release.

This script is meant to be used with tellico (http://periapsis.org/tellico) as an external data source program.
It allows searching through the bedetheque.com web database. You are welcome to report any bug to Raphaël Fischer, such as missing or wrong information, when comparing to linked bedetheque.com page.
"""

import sys, os, re, random, string, socket, time
try:
        import hashlib
        md = hashlib.md5()
except ImportError:
        # for Python << 2.5
        import md5
        md = md5.new()
import urllib, urllib2, time, base64
import xml.dom.minidom, locale
socket.setdefaulttimeout(1)
loc = re.search('\.([^\.]*)', locale.setlocale(locale.LC_ALL, ''))
if loc:
        local = loc.group(1)
else:
        local = 'UTF-8'

XML_HEADER = """<?xml version="1.0" encoding="UTF-8"?>"""
DOCTYPE = """<!DOCTYPE tellico PUBLIC "-//Robby Stephenson/DTD Tellico V9.0//EN" "http://periapsis.org/tellico/dtd/v9/tellico.dtd">"""
NULLSTRING = ''

VERSION = "0.5"

def genMD5():
        obj = md
        float = random.random()
        obj.update(str(float))
        return obj.hexdigest()

class BasicTellicoDOM:
        """
        This class manages tellico's XML data model (DOM)
        """
        def __init__(self):
                self.__dict = {'Á' : '&Aacute;', 'á' : '&aacute;', 'Â' : '&Acirc;', 'â' : '&acirc;', 'À' : '&Agrave;', 'à' : '&agrave;', 'Å' : '&Aring;', 'å' : '&aring;', 'Ã' : '&Atilde;', 'ã' : '&atilde;', 'Ä' : '&Auml;', 'ä' : '&auml;', '´' : '&acute;', 'Æ' : '&AElig;', 'æ' : '&aelig;', '¦' : '&brvbar;', 'Ç' : '&Ccedil;', 'ç' : '&ccedil;', '¸' : '&cedil;', '»' : '&raquo;', '«' : '&laquo;', '©' : '&copy;', '²' : '&sup2;', 'É' : '&Eacute;', 'é' : '&eacute;', 'Ê' : '&Ecirc;', 'ê' : '&ecirc;', 'È' : '&Egrave;', 'è' : '&egrave;', 'Ë' : '&Euml;', 'ë' : '&euml;', ' ' : '&nbsp;', '&' : '&amp;', 'Ð' : '&ETH;', 'ð' : '&eth;', '¡' : '&iexcl;', '¾' : '&frac34;', '½' : '&frac12;', '¼' : '&frac14;', 'Í' : '&Iacute;', 'í' : '&iacute;', 'Î' : '&Icirc;', 'î' : '&icirc;', 'Ì' : '&Igrave;', 'ì' : '&igrave;', 'Ï' : '&Iuml;', 'ï' : '&iuml;', '<' : '&lt;', '¯' : '&macr;', '®' : '&reg;', 'Ñ' : '&Ntilde;', 'ñ' : '&ntilde;', 'Ó' : '&Oacute;', 'ó' : '&oacute;', 'Ô' : '&Ocirc;', 'ô' : '&ocirc;', 'Ò' : '&Ograve;', 'ò' : '&ograve;', 'Ø' : '&Oslash;', 'ø' : '&oslash;', 'Õ' : '&Otilde;', 'õ' : '&otilde;', 'Ö' : '&Ouml;', 'ö' : '&ouml;', 'ª' : '&ordf;', 'º' : '&ordm;', '±' : '&plusmn;', '¿' : '&iquest;', '·' : '&middot;', 'ß' : '&szlig;', '°' : '&deg;', '÷' : '&divide;', 'µ' : '&micro;', '×' : '&times;', '¬' : '&not;', '¶' : '&para;', '§' : '&sect;', '¥' : '&yen;', '>' : '&gt;', '¢' : '&cent;', '£' : '&pound;', '¤' : '&curren;', 'Þ' : '&THORN;', 'þ' : '&thorn;', '¨' : '&uml;', '³' : '&sup3;', 'Ú' : '&Uacute;', 'ú' : '&uacute;', 'Û' : '&Ucirc;', 'û' : '&ucirc;', 'Ù' : '&Ugrave;', 'ù' : '&ugrave;', 'Ü' : '&Uuml;', 'ü' : '&uuml;', '¹' : '&sup1;', 'Ý' : '&Yacute;', 'ý' : '&yacute;', 'ÿ' : '&yuml;'}

                self.__doc = xml.dom.minidom.Document()
                self.__root = self.__doc.createElement('tellico')
                self.__root.setAttribute('xmlns', 'http://periapsis.org/tellico/')
                self.__root.setAttribute('syntaxVersion', '9')

                self.__collection = self.__doc.createElement('collection')
                self.__collection.setAttribute('title', 'My Comics')
                self.__collection.setAttribute('type', '6')

                self.__fields = self.__doc.createElement('fields')
                # Add all default (standard) fields
                self.__dfltField = self.__doc.createElement('field')
                self.__dfltField.setAttribute('name', '_default')
                self.__fields.appendChild(self.__dfltField)
                self.newField(field = {'name': 'artist', 'title': 'Dessinateur', 'flags': '7', 'category': 'Général', 'format': '2', 'type': '1'})
                self.newField(field = {'flags' : '7', 'title' : 'Coloriste', 'category' : 'Général', 'format': '2', 'type': '1', 'name': 'coloriste'})
                self.newField(field = {'flags' : '0', 'title' : 'ISBN', 'category' : 'Publication', 'format' : '4', 'type' : '1', 'name' : 'isbn', 'description' : 'Numéro ISBN'})
                self.newField(field = {'flags' : '0', 'title' : 'Estimation', 'category' : 'Classification', 'format' : '4', 'type' : '1', 'name' : 'estimation', 'description' : 'Cote de l\'exemplaire'})
                self.newField(field = {'flags' : '0', 'title' : 'Dépot légal', 'category' : 'Publication', 'format' : '4', 'type' : '1', 'name' : 'depotlegal', 'description' : 'Dépot légal'})
                self.newField(field = {'flags' : '0', 'title' : 'Achevé d\'imprimer', 'category' : 'Publication', 'format' : '4', 'type' : '1', 'name' : 'acheveimprimer', 'description' : 'Date d\'impression'})
                self.newField(field = {'flags' : '0', 'title' : 'Quatrième de couverture', 'category' : 'Quatrième de couverture', 'format' : '4', 'type' : '10', 'name' : 'back', 'description' : 'Quatrième de couverture'})
                self.newField(field = {'flags' : '0', 'title' : 'Planche', 'category' : 'Planche', 'format' : '4', 'type' : '10', 'name' : 'planche', 'description' : 'Planche'})
                self.newField(field = {'flags' : '2', 'title' : 'Intégrale', 'category' : 'Classification', 'format' : '4', 'type' : '4', 'name' : 'class-int'})
                self.newField(field = {'flags' : '2', 'title' : 'Édition Originale', 'category' : 'Classification', 'format' : '4', 'type' : '4', 'name' : 'class-eo'})
                self.newField(field = {'flags' : '2', 'title' : 'Tirage de Tête', 'category' : 'Classification', 'format' : '4', 'type' : '4', 'name' : 'class-tt'})
                self.newField(field = {'flags' : '2', 'title' : 'Broché', 'category' : 'Classification', 'format' : '4', 'type' : '4', 'name' : 'class-br'})
                self.newField(field = {'flags' : '2', 'title' : 'Tirage Limité', 'category' : 'Classification', 'format' : '4', 'type' : '4', 'name' : 'class-tl'})
                self.newField(field = {'flags' : '0', 'title' : 'Lien BEL', 'category' : 'Classification', 'format' : '4', 'type' : '7', 'name' : 'lien-bel', 'description' : 'URL de la page de la Base En Ligne de bedetheque.com de cet album'})
                field = {'name': 'note', 'title': 'Note', 'flags': '0', 'category': 'Personnel', 'format': '4', 'type': '14', 'description': 'Avis sur l\'album'}
                self.__customField = self.__doc.createElement('field')
                for item, value in field.iteritems():
                        self.__customField.setAttribute(item, value)
                minNode = self.__doc.createElement('prop')
                minNode.setAttribute('name', 'minimum')
                minNode.appendChild(self.__doc.createTextNode('0'))
                self.__customField.appendChild(minNode)
                maxNode = self.__doc.createElement('prop')
                maxNode.setAttribute('name', 'maximum')
                maxNode.appendChild(self.__doc.createTextNode('10'))
                self.__customField.appendChild(maxNode)
                self.__fields.appendChild(self.__customField)
                self.__collection.appendChild(self.__fields)

                self.__images = self.__doc.createElement('images')
                self.__root.appendChild(self.__collection)
                self.__doc.appendChild(self.__root)
                self.__currentId = 0

        def newField(self, field):
                self.__customField = self.__doc.createElement('field')
                for item, value in field.iteritems():
                        if local != "UTF-8":
                                value = unicode(value, 'UTF-8').encode(local)
                        self.__customField.setAttribute(item, value)
                self.__fields.appendChild(self.__customField)
                self.__collection.appendChild(self.__fields)

        def recode(self, chaine):
                for vers, de in self.__dict.iteritems():
                        chaine = re.sub(de, vers, chaine)
                if local != "UTF-8":
                        try:
                                chaine = unicode(chaine, 'UTF-8').encode(local)
                        except:
                                pass
                return chaine

        def addEntry(self, albumData):
                """
                Add a comic entry.
                Returns an entry node instance
                """
                d = albumData
                entryNode = self.__doc.createElement('entry')
                entryNode.setAttribute('id', str(self.__currentId))
                fields = {      'title' : 'title',
                                'depotlegal' : 'depotlegal',
                                'country' : 'country',
                                'publisher' : 'publisher',
                                'language' : 'language',
                                'issue' : 'issue',
                                'writer' : 'writer',
                                'artist' : 'dessin',
                                'coloriste' : 'coloriste',
                                'isbn' : 'isbn',
                                'series' : 'serie',
                                'edition' : 'collec',
                                'pages' : 'pages',
                                'language' : 'language',
                                'genre' : 'genre',
                                'note' : 'note',
                                'estimation' : 'cote',
                                'acheveimprimer' : 'acheveimprimer',
                                'comments' : 'comments',
                                'pub_year' : 'year',
                                'class-int' : 'class-int',
                                'class-eo' : 'class-eo',
                                'class-tt' : 'class-tt',
                                'class-br' : 'class-br',
                                'class-tl' : 'class-tl',
                                'lien-bel' : 'bdgest'
                                }
                s = ''
                comments = [('synopsis', "<b>Synopsis série : </b>%s"),
                                                        ('infos', "<b>Infos édition : </b>%s"),
                                                        ('texte_chronique', '%s'),
                                                        ]
                for key, texte in comments:
                        if key in d.keys():
                                if len(d[key]) > 0:
                                        s = "%s%s" % (s, texte % d[key]) if len(s) == 0 else "%s<br/>%s" % (s, texte % d[key])
                d['comments'] = s
                for image in ('cover', 'back', 'planche'):
                        if image in d.keys():
                                if d[image]:
                                        imageNode = self.__doc.createElement('image')
                                        imageNode.setAttribute('format', 'JPEG')
                                        imageNode.setAttribute('id', d[image][0])
                                        #imageNode.appendChild(self.__doc.createTextNode(unicode(d[image][1], 'latin-1').encode('utf-8')))
                                        if local != "UTF-8":
                                                try:
                                                        d[image][1] = unicode(value, 'UTF-8').encode(d[image][1])
                                                except:
                                                        pass
                                        imageNode.appendChild(self.__doc.createTextNode(d[image][1]))
                                        newNode = self.__doc.createElement(image)
                                        newNode.appendChild(self.__doc.createTextNode(d[image][0]))
                                        entryNode.appendChild(newNode)
                                        self.__images.appendChild(imageNode)
                for name, value in fields.iteritems():
                        if value in d.keys():
                                Node = self.__doc.createElement(name)
                                Node.appendChild(self.__doc.createTextNode(self.recode(d[value])))
                                entryNode.appendChild(Node)
                self.__collection.appendChild(entryNode)
                self.__currentId += 1
                return entryNode

        def printEntry(self, nEntry):
                """
                Prints entry's XML content to stdout
                """
                try:
                        print nEntry.toxml()
                except:
                        print sys.stderr, "Error while outputing XML content from entry to Tellico"

        def printXMLTree(self):
                """
                Outputs XML content to stdout
                """
                self.__collection.appendChild(self.__images)
                print XML_HEADER; print DOCTYPE
                print self.__root.toxml()

class BDthequeParser:
        class HTMLrequest:
                """
                Defines some procedures to enable use of proxies
                """
                def __init__(self, verbose):
                        self.error_pattern = {
                                "www.bedetheque.com" : 'IP interdite pour abus. Contactez',
                                "www.bdgest.com" : 'IP interdite pour abus. Contactez',
                                "google" : "<a href=\"/intl/fr/about\.html\">[^ ]* propos de Google</a>",
                                "all" : "http-equiv=\"?refresh\"?"}
                        self.PO_error = {}
                        for site, pattern in self.error_pattern.iteritems():
                                self.PO_error[site] = re.compile(pattern, 18)
                        self.is_excluded = {'www.bedetheque.com' : False}
                        self.URLfrom = {"google" : [0, 0], "proxy" : [0, 0], 'www.bedetheque.com' : [0, 0]}
                        self.access_limit_to_site = 10
                        self.local_list = "/tmp/tellico_proxy_list.txt"
                        self.verbose = verbose
                        self.proxylist = []

                def _getproxylist(self):
                        """
                        Obtain a list of proxies from external sites
                        """
                        if self.URLfrom["proxy"][0] == 0:
                                #Trying to access recent proxy list
                                try:
                                        f = open(self.local_list, 'r')
                                        tmp = f.read()
                                        f.close()
                                except:
                                        self.vprint(1, 'File protection error : unable to read temporary file %s' % self.local_list)
                                        tmp= ''
                                        pass
                                if len(tmp):
                                        update_time = re.search('Last updated : ([0-9]+)', tmp)
                                        if int(update_time.group(1)) > int(time.time()) - 3600:
                                                self.proxylist = re.findall('([0-9.]*):([0-9]*)\t(.*?)\n?', tmp)
                                                self.vprint(1, 'Local list is only %s min old. There\'s %s proxies in it' % (int(int(time.time() - int(update_time.group(1)))/60), len(self.proxylist)))
                                        else:
                                                self.vprint(1, 'Local list is already %s min old.' % (int(int(time.time() - int(update_time.group(1)))/60)))
                        # Otherwise, obtain it from remote lists
                        if not len(self.proxylist):
                          self.freeproxylists =[
                                  ("http://www.free-proxy.fr/", '<tr><td width="50%" align="left">([0-9.]*):([0-9]*)</td><td width="50%" align="center">[^<]*</td></tr>'),
                                  ('http://www.aliveproxy.com/fr-proxy-list/',  '([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+):([0-9]+)'),
                                  ('http://proxynext.com/proxylist1.php', '([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)</td>\s*?<td.*?>([0-9]+)</td>'),
                                  ('http://www.proxy4free.com/page1.html', '([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)</td>\s*?<td.*?>([0-9]+)</td>')
                                  ]
                          try:
                                  u = urllib2.urlopen("http://fischer.tellico.free.fr/freeproxylists.txt", None, 1)
                                  update = u.read()
                                  u.close()
                                  self.vprint(1, "Looking for new sites on remote list")
                          except:
                                  update = ''
                                  self.vprint(1, "Update file was not found")
                          if len(update):
                                  newlists = re.findall('^(http.*?)\t(.*?)\n', update)
                                  self.vprint(1, "%s lists of lists found" % len(newlists))
                                  for url, pat in newlists:
                                          updated = False
                                          for url2, pat2 in self.freeproxylists:
                                                  if (url2 == url and pat2 == pat):
                                                          updated = True
                                          if not updated:
                                                  self.freeproxylists.append((url, pat))
                                                  self.vprint(1, "Site : %s\nPattern : %s" % (url, pat))
                          self.proxylist = []
                          for url, pattern in self.freeproxylists:
                                  self.vprint(1, "Looking for proxies in %s" % url)
                                  request = urllib2.Request(url)
                                  request.add_header('User-Agent','Mozilla/4.0')
                                  opener = urllib2.build_opener()
                                  urllib2.install_opener(opener)
                                  try:
                                          pagelist = urllib2.urlopen(request).read()
                                  except:
                                          pagelist = ''
                                          self.vprint(1, "This proxy URL is invalid")
                                  if len(pagelist):
                                          newproxylist = re.findall(pattern,  pagelist)
                                          if len(newproxylist):
                                                  self.vprint(1, "%s new proxies found" % len(newproxylist))
                                          else:
                                                  self.vprint(1, "The pattern does not return a value")
                                          for proxy_IP, proxy_port in newproxylist:
                                                  self.proxylist.append((proxy_IP, proxy_port, url))
                        self.saveproxylist()
                        self.newproxy()

                def newproxy(self):
                        """Defining a new random proxy"""
                        self.proxy_number = random.randint(0, len(self.proxylist) - 1)
                        self.vprint(3, self.proxylist[self.proxy_number])
                        proxy_IP, proxy_port, self.referer = self.proxylist[self.proxy_number]
                        proxy_support = urllib2.ProxyHandler({"http" : "%s:%s" % (proxy_IP, proxy_port)})
                        self.proxy_opener = urllib2.build_opener(proxy_support)

                def saveproxylist(self):
                          """Saving proxy list on local file"""
                          s = 'Last updated : %s (%s)\n' % (time.time(), time.strftime("%a, %d %b %Y %H:%M:%S"))
                          for IP, port, url in self.proxylist:
                                  s = "%s\n%s:%s\t%s" % (s, IP, port, url)
                          try:
                                  f = open(self.local_list, 'w')
                                  f.write(s)
                                  f.close()
                          except:
                                  self.vprint(1, 'File protection error : unable to write temporary file %s' % self.local_list)
                                  pass

                def getPage(self, site, request, proxy, refreshed):
                        """Actually download the page using the specifed method"""
                        try:
                                if proxy:
                                        page = self.proxy_opener.open(request).read()
                                else:
                                        page = urllib2.urlopen(request).read()
                        except:
                                return ''
                        if site in self.PO_error.keys() and self.PO_error[site].search(page):
                                self.URLfrom[site][0] += 1
                                if not proxy:
                                        self.is_excluded[site] = True
                                return ''
                        elif not refreshed and self.PO_error["all"].search(page):
                                request.add_header('Referer', 'url')
                                page = self.getPage(site, request, proxy, 1)
                                if not len(page):
                                        return ''
                        self.URLfrom[site][1] += 1
                        f = open("last_downloaded_page.html", 'w')
                        f.write(page)
                        f.close()
                        return page

                def getHTMLContent(self, url, enable_cache):
                        """     Fetch HTML data from url        """
                        page = ''
                        site = re.search('http://([^/]*)', url)
                        if site:
                                site = site.group(1)
                        else:
                                return ''
                        if site not in self.URLfrom.keys():
                                self.URLfrom[site] = [0, 0]
                                self.is_excluded[site] = False

                        if self.URLfrom[site][1] >= self.access_limit_to_site:
                                self.is_excluded[site] = True
                        if not self.is_excluded[site]:
                                self.vprint(1, "Trying to open directly:\n\t%s" % url)
                                request = urllib2.Request(url)
                                request.add_header('User-Agent', 'Mozilla/4.0  (compatible; MSIE 5.5; Windows NT)')
                                request.add_header('Referer', '')
                                page = self.getPage(site, request, 0, 0)
                                if len(page):
                                        try:
                                                page = unicode(page,"iso-8859-1").encode( "UTF-8")
                                        except:
                                                self.vprint(1, "Error while encoding latin-1 -> UTF-8")
                                        return page
                                else:
                                        pass

                        if enable_cache:
                                self.vprint(1, "Trying to open from google cache:\n\t%s" % url)
                                request = urllib2.Request('http://www.google.com/search?hl=fr&q=%s' % urllib.quote('cache:%s' % url))
                                request.add_header('User-Agent', 'Mozilla/4.0  (compatible; MSIE 5.5; Windows NT)')
                                request.add_header('Referer', '')
                                page = self.getPage("google", request, 0, 0)
                                if len(page):
                                        #Google semble convertir les pages en UTF-8 !
                                        return page
                                else:
                                        pass

                        self.vprint(1, "Trying to open using proxy:\n\t%s" % url)
                        if len(self.proxylist) == 0:
                                self._getproxylist()
                        request = urllib2.Request(url)
                        request.add_header('User-Agent', 'Mozilla/4.0  (compatible; MSIE 5.5; Windows NT)')
                        request.add_header('Referer', self.referer)
                        len_before = len(self.proxylist)
                        while (not len(page)) and len(self.proxylist):
                                page = self.getPage(site, request, 1, 0)
                                if not len(page):
                                        self.URLfrom["proxy"][0] += 1
                                        self.vprint(3, "Proxy did not work")
                                        self.proxylist.pop(self.proxy_number)
                                        if len(self.proxylist):
                                                self.newproxy()
                                else:
                                        break
                                self.vprint(2, "Looking for a working proxy")
                        if len(page):
                                self.URLfrom["proxy"][1] += 1
                                if self.URLfrom["proxy"][1] % self.access_limit_to_site == 0:
                                        #Switching proxy without killing it
                                        self.newproxy()
                                if len(self.proxylist) < len_before:
                                        self.vprint(1, "%s remaining proxies" % len(self.proxylist))
                                        self.saveproxylist()
                                f = open("last_downloaded_page.html", 'w')
                                f.write(page)
                                f.close()
                                try:
                                        page = unicode(page,"iso-8859-1").encode( "UTF-8")
                                except:
                                        self.vprint(1, "Error while encoding latin-1 -> UTF-8")
                                return page
                        self.vprint(1, "No more proxies left. Failed fetching required URL")
                        self.saveproxylist()
                        return ''

                def vprint(self, v, st):
                        if (v <= self.verbose):
                                try:
                                        print unicode(st, "UTF-8").encode(local)
                                except:
                                        print st
                                return True
                        else:
                                return False

        def __init__(self):
                self.__maxresult = 20
                self.__comment = ''
                self.__options  = {'-t' : '', '-a' : '', '-isbn' : '', '-v' : 0, '-q' : 0, '-dl' : '', '-r' : 0, '-l' : -1, '-i' : 0, '-c' : 1, '-f' : 20, '-lk' : ''}
                self.__baseURL   = 'http://www.bedetheque.com'
                self.__basePath  = '/'
                self.__searchmixedURL = '/index.php?R=1&RechTexte=%s&Recherche=OK&RechSerie=%s&RechAuteur=%s&RechISBN=%s&RechDL=%s'
                self.__searchtitleURL = '/index.php?R=1&RechSerie=%s'
                self.__searchserieURL = '/list_series.php?FiltreSerie=%s'
                self.__searchauteurURL = '/list_auteurs.php?FiltreAuteur=%s'
                self.__bdgest = 'http://bdgest.com/'
                self.__searchCritiqueURL = 'http://www.bdgest.com/critique.php?cherchecrit=%s'
                self.__coverPath = 'thb_couv/'
                self.__SerieURL = self.__baseURL + self.__basePath
                self.__searchExps = [
                                                '<A HREF="(?P<url>album-(?P<id>[0-9]+)-[^"]*?)"[^>]*?><i>(?P<serie>[^<]*)</i> *-(?P<issue>[^ ]*?)- *(?P<title>[^<]*?)</A></TD><TD.*?>(?P<date>.*?)</TD></tr>',
                                                '<A HREF="(?P<url>album-(?P<albumid>[0-9]+)-[^"]*?)"[^>]*?><i>(?P<serie>[^<]*)</i> *(?P<issue>)(?P<title>[^ -][^<]*?)</A></TD><TD.*?>(?P<date>.*?)</TD></tr>']
                self.__regExps = {
                                                'depotlegal' : 'Dépot légal :</td><td>([0-9]{2})/([0-9]{4})',
                                                'acheveimprimer' : 'Achevé impr. :</td><td>([^<]*)',
                                                'dessin' : '<tr><td>Dessin :</td><td><A HREF="[^>]+>([^<]*)</A></td></tr>',
                                                'writer' : '<tr><td>Scénario :</td><td><A HREF=[^>]+>([^<]+)</A></td></tr>',
                                                'coloriste' : '<tr><td>Couleurs :</td><td><A [^>]+>([^<]*)</A></td></tr>',
                                                'publisher' : '<tr><td>Editeur :</td><td>([^<]*)</td></tr>',
                                                'collec' : '<tr><td>Collection : </td><td>([^<]*)</td></tr>',
                                                'isbn' : '<tr><td>ISBN :</td><td>([^<]*)</td></tr>',
                                                'infos' : '<b>Info édition : </b><i>(.*?)</i></div>',
                                                'synopsis' : '<b>Résumé de la série : </b><i>(.*?)</i></div>',
                                                'cote' : '<tr><td>Estimation :</td><td>([^<]*)</td></tr>',
                                                'pages' : '<tr><td>Planches :</td><td>([0-9]*)</td></tr>',
                                                'number' : '<a name="([0-9]+)">',
                                                'cover' : '<A HREF="(Couvertures/[^"]*)"',
                                                'back' : '<a href="(Versos/[^"]*)"',
                                                'planche' : '<a href="(Planches/[^"]*)"',
                                                'note' : '<iframe src="(frame_rating\.php\?Id=[A-Z0-9]+)',
                                                'class-int' : '<IMG SRC="images/Inte\.png"',
                                                'class-eo' : '<IMG SRC="images/Edo\.png"',
                                                'class-tt' : '<IMG SRC="images/TT\.png"',
                                                'class-br' : '<IMG SRC="images/Broche.png"'
                                                }
                self.__regexp_alt = {
                                                'coloriste' : '<tr><td>Couleurs :</td><td><A [^>]+>([^<]*)</font></A></td></tr>',
                                                'dessin'                                : '<tr><td>Dessin :</td><td><A HREF="[^>]*><font [^>]*>([^<]*)</font></A></td></tr>',
                                                'publisher'                     : '<tr><td>Editeur :</td><td><a[^>]*>([^<]*)</a></td></tr>',
                                                }
                self.__chronique = {
                                                'lien' : '<A HREF="http://www\.bdgest\.com/(?P<chro_url>chronique[^"]*)" title="Voir la critique"><FONT[^>]*><b>(?P<serie>[^<]*)</b></font></A> .*?(?:<br>)?<font.*?>(?P<title>.*?)</font>',
                                                'texte'                                                 : '<a href=".*?critique\.php\?a=[0-9]+"><[iu]>(?P<who>[^<]*)</[iu]></a>&nbsp;.*<img [^>]*>(?P<text>.*?)<b>» Votre avis nous intéresse'}
                self.__serieRE = {
                                                'inner'                         : '<table width="100%" cellpadding="0" cellspacing="0">\s*<tr>\s*<td>\s*<IMG SRC="images/spacer_h.gif" BORDER=0 WIDTH=0 HEIGHT=5><BR>\s*<TABLE WIDTH="100%" BORDER=0  CELLPADDING=0 CELLSPACING=0>(?P<page>.*)<table><tr><td height="5"></td></tr></table>\s*</td></tr></table>\s*<IMG SRC="images/spacer_h.gif" BORDER=0 WIDTH=0 HEIGHT=5>',
                                                'item'                                          : '<td align="left" valign="top" WIDTH="[0-9]*"><div class="img-shadow">.*?<div id="ErreurAlbum',
                                                'split'                                         : '<td align[^>]*><table[^>]*>',
                                                'reeditions'                            : '<span class="[^"]*"><a href="(?P<fiche>fiche-[0-9]+-[^"]*)"',
                                                'serie' : '<h2><b>Série : <a href="serie-[0-9]*-.*?">([^<]*)</a></b></h2>',
                                                'title' : '<h1>([^<]*)</h1>',
                                                'issue' : '<title>.*? -([0-9]*)- [^<]*</title>'
                                                }
                self.__reeditionsRE = {
                                                'depotlegal'                    : 'Dépot légal :</td><td>([0-9]{2})/([0-9]{4})',
                                                'acheveimprimer'        : 'Achevé impr. :</td><td>([^<]*)',
                                                'dessin'                                        : '<tr><td[^>]*>Dessin :</td><td><A HREF="[^>]+>([^<]*)</A></td></tr>',
                                                'writer'                                        : '<tr><td[^>]*>Scénario :</td><td><A HREF=[^>]+>([^<]+)</A></td></tr>',
                                                'coloriste'                             : '<tr><td[^>]*>Couleurs :</td><td><A [^>]+>([^<]*)</A></td></tr>',
                                                'publisher'                     : '<tr><td>Editeur :</td><td>([^<]*)</td></tr>',
                                                'collec'                                        : '<tr><td>Collection : </td><td>([^<]*)</td></tr>',
                                                'isbn'                                  : '<tr><td>ISBN :</td><td>([^<]*)</td></tr>',
                                                'infos'                                 : '<b>Info édition : </b><i>([^<]*)</i></div>',
                                                'cote'                                  : '<tr><td>Estimation :</td><td>([^<]*)</td></tr>',
                                                'pages'                                         : '<tr><td>Planches :</td><td>([0-9]*)</td></tr>',
                                                'number'                                : '<td>Identifiant :</td><td>([0-9]+)</td>',
                                                'cover'                                 : '<IMG SRC="(Couvertures/[^"]*)"',
                                                'back'                                  : '<a href="(Versos/[^"]*)"',
                                                'planche'                               : '<a href="(Planches/[^"]*)"',
                                                'class-int'                             : '<IMG SRC="images/Inte\.png"',
                                                'class-eo'                              : '<IMG SRC="images/Edo\.png"',
                                                'class-tt'                              : '<IMG SRC="images/TT\.png"',
                                                'class-br'                              : '<IMG SRC="images/Broche.png"',
                                                'country'                               : '<tr><td>Origine :</td><td>([^</]*)/?[^<]*</td></tr>',
                                                'language'                      : '<tr><td>Style :</td><td>[^</]*/?([^</]*)</td></tr>',
                                                'synopsis'                      : '<td><div align="justify"><b>Résumé : </b></font><i>(.*?)</i></font></div></td></tr>',
                                                'genre'                                 : '<tr><td>Style :</td><td>([^<]*)</td></tr>',
                                                'serie'                                 : '<b><a href="http://www\.bedetheque\.com/serie[^"]*"[^>]*>([^<]*)</a>'
                                                }
                self.__reeditionsRE2 = {
                                                'cover'                                 : '<IMG SRC="%s[^"]*)' % 'http://www\.bedetheque\.com/(Couvertures/',
                                                'back'                                  : '<A HREF="http://www\.bedetheque\.com/(Versos/[^"]*)"',
                                                'planche'                               : '<A HREF="http://www\.bedetheque\.com/(Planches/[^"]*)"',
                                                }

                # Compile patterns objects
                self.__chronPO = {}
                for k, pattern in self.__chronique.iteritems():
                        self.__chronPO[k] = re.compile(pattern, 18)
                self.__regExpsPO = {}
                for k, pattern in self.__regExps.iteritems():
                        self.__regExpsPO[k] = re.compile(pattern, 18)

                self.__reeditionsPO = {}
                for k, pattern in self.__reeditionsRE.iteritems():
                        self.__reeditionsPO[k] = re.compile(pattern, 18)
                self.__reeditionsPO2 = {}
                for k, pattern in self.__reeditionsRE2.iteritems():
                        self.__reeditionsPO2[k] = re.compile(pattern, 18)

                #self.__seriePO = {}
                #for k, pattern in self.__regexp_serie.iteritems():
                        #self.__seriePO[k] = re.compile(pattern, 18)
                self.__seriePO = {}
                for k, pattern in self.__serieRE.iteritems():
                        self.__seriePO[k] = re.compile(pattern, 16)
                self.__regExps_altPO = {}
                for k, pattern in self.__regexp_alt.iteritems():
                        self.__regExps_altPO[k] = re.compile(pattern, 18)

                self.__domTree = BasicTellicoDOM()

        def __fetchAlbumInfo(self, page):
                page = re.sub('</a></td></tr><tr><td></td><td><a [^>]*>(?i)', ';', page)
                page = re.sub('</font>', '', page)
                data, matches = {}, {}
                for name, po in self.__regExpsPO.iteritems():
                        data[name] = ''
                        matches[name] = po.findall(page)
                        if not matches[name] and name in self.__regExps_altPO.keys():
                                matches[name] = self.__regExps_altPO[name].findall(page)
                        if matches[name]:
                                self.vprint(2, '%s : %s' % (name, matches[name]))
                                for item in matches[name]:
                                        if name == 'note':
                                                notepage = self.getURL.getHTMLContent('%s/%s' % (self.__baseURL, item), 1)
                                                value = re.search('<p class="static">Note: <strong> ([0-9]+)\.([0-9]?)</strong>/10 \([0-9]+ votes?\)', notepage)
                                                if value:
                                                  entier, decimale = value.group(1), value.group(2)
                                                  data[name] = str(int(entier) + 1 if int(decimale) >= 5 else int(entier))
                                        elif name == 'depotlegal':
                                                mois, annee = item
                                                data[name] = '%s/%s' % (annee, mois)
                                                data['year'] = str(annee)
                                                data['mois'] = str(mois)
                                        elif re.match('class-', name):
                                                data[name] = 'true'
                                        else:
                                                if len(data[name]) > 0:
                                                        data[name] = "%s;%s"% (data[name], item.strip())
                                                else:
                                                        data[name] = item.strip()
                return data

        def __fetchAlbumFromFiche(self, fiche):
                self.__data = self.getURL.getHTMLContent(fiche, 1)
                page = re.sub('</[fF][oO][nN][tT]>', '', self.__data)
                data, matches = {'year' : '', 'mois' : ''}, {}
                for name, po in self.__reeditionsPO.iteritems():
                        data[name] = ''
                        matches[name] = po.findall(page)
                        if not matches[name] and name in self.__reeditionsPO2.keys():
                                self.vprint(1, 'Used alt. RegExp for %s' % name)
                                matches[name] = self.__reeditionsPO2[name].findall(page)
                        if matches[name]:
                                for item in matches[name]:
                                        if len(data[name]) > 0:
                                                data[name] = "%s;%s" % (data[name], item.strip())
                                        else:
                                                if name == 'depotlegal':
                                                        mois, annee = item
                                                        data[name] = '%s/%s' % (annee, mois)
                                                        data['year'] = str(annee)
                                                        data['mois'] = str(mois)
                                                elif re.match('class-', name):
                                                        data[name] = 'true'
                                                else:
                                                        data[name] = item.strip()
                return data

        def __getChronicleList(self, serie):
                if serie in self.__all_chronicles.keys():
                        self.__chronicles = self.__all_chronicles[serie]
                else:
                        self.__data =  self.getURL.getHTMLContent(self.__searchCritiqueURL % urllib.quote(unicode(serie, 'UTF-8').encode('latin-1')), 1)
                        chro_node = re.findall(self.__chronique['lien'], self.__data)
                        self.__chronicles = {}
                        self.vprint(1, '%s critics found in the serie %s' % (len(chro_node), serie))
                        for chro_url, serie_c, titre in chro_node:
                                self.__chronicles[chro_url] = {'url' : chro_url, 'serie' : serie_c, 'title' : titre}
                                self.vprint(1, '\tSerie : %s\n\tTitle : %s\n\tURL : %s' % (serie_c, titre, chro_url))
                        self.__all_chronicles[serie] =  self.__chronicles

        def __getAlbums(self):
                """ Executing search """
                if len(self.__directlink):
                        bdgest = re.search('http://www\.bedetheque\.com/(album-([0-9]*)-.*?\.html)', self.__directlink)
                        if bdgest:
                                albumid = bdgest.group(2)
                                self.__directlink = bdgest.group(1)
                        else:
                                self.__directlink = ''
                if len(self.__directlink):
                        #Skiping search, since id is given
                        album = 1
                        links = {albumid : {'url' : self.__directlink, 'title' : self.__title, 'serie' : '', 'issue' : [(self.__DL, '')]}}
                else:
                        self.__data = self.getURL.getHTMLContent("%s%s%s" % (
                                self.__baseURL,
                                self.__searchmixedURL % (
                                        urllib.quote(''),
                                        urllib.quote(unicode(self.__title, 'UTF-8').encode('latin-1')),
                                        urllib.quote(unicode(self.__author, 'UTF-8').encode('latin-1')),
                                        urllib.quote(unicode(self.__isbn, 'UTF-8').encode('latin-1')),
                                        urllib.quote(unicode(self.__DL, 'UTF-8').encode('latin-1'))),
                                '&Reeditions=on' if self.__reeditions else ''), 0)
                        # Fetching album and serie list
                        links = {}
                        for reg in self.__searchExps:
                                album_list = re.findall(reg, self.__data, 18)
                                for item in album_list:
                                        url, albumid, serie, issue, title, dl = item
                                        serie = re.sub('\([^)]*\)', '', serie).strip()
                                        self.vprint(2, '(1) %s ;\t%s \t| %s \n\t(id : %s [%s]])' % (serie, issue, title, albumid, url))
                                        if albumid not in links.keys():
                                                links[albumid] = {'url' : url, 'title' : title, 'serie' : serie, 'issue' : [(dl, issue)]}
                                        else:
                                                links[albumid]['issue'].append((dl, issue))
                                self.vprint(1, "%s album(s)" % len(album_list))
                        album = len(album_list)
                nbr = 0
                self.__noimage = album > self.__limit and (self.__limit != -1 or (album > 30))
                if self.__noimage:
                        self.vprint(1, 'Album number exceeded ()%s to %s) images will not be fetched' % (album, self.__limit))
                self.__chronicles = {}
                self.__data_serie = {}
                self.__albums = []
                self.__all_chronicles = {}
                linksorder = links.keys()
                linksorder.sort()
                for albumid in linksorder:
                        mixed = links[albumid]
                        url = mixed['url']
                        title = mixed['title']
                        serie = mixed['serie']
                        issues = mixed['issue']
                        if self.__fast > 0 and len(links) > self.__fast:
                                # Only the results from the initial search are returned
                                for dl, issue in issues:
                                        data = {'title' : title, 'serie' : serie, 'depotlegal' : dl, "bdgest" : '%s/%s' % (self.__baseURL, url)}
                                        n = re.search('[0-9]*', issue)
                                        #data['texte_chronique'] = "Demandez la mise à jour de l'album pour obtenir le reste des informations"
                                        if n:
                                                data['issue'] = n.group(0)
                                        date = re.match('([0-9]{2})/([0-9]{4})', dl)
                                        if date:
                                                data['mois'] = str(date.group(1))
                                                data['year'] = str(date.group(2))
                                        self.__domTree.addEntry(data)
                                        nbr += 1
                        else:
                                # One album at a time, but possibly several editions
                                self.__data = self.getURL.getHTMLContent('%s/%s' % (self.__baseURL, url), 1)
                                #Fetching info on the album on the first page
                                EO_data = {}
                                for key in ['serie', 'title', 'issue']:
                                        tmp = self.__seriePO[key].search(self.__data)
                                        if tmp:
                                                EO_data[key] = tmp.group(1)
                                        else:
                                                if len(mixed[key]):
                                                        if key == 'issue':
                                                                dl, issue = mixed['issue'][0]
                                                                n = re.search('[0-9]*', issue)
                                                                if n:
                                                                        EO_data['issue'] = n.group(0)
                                                        else:
                                                                EO_data[key] = mixed[key]
                                                else:
                                                        EO_data[key] = ''
                                title, serie, issue = EO_data['title'], EO_data['serie'], EO_data['issue']
                                
                                matchList = self.__seriePO['item'].findall(self.__data)
                                self.vprint(2, '%s \t\t(%s editions prévues, \t%s trouvées)' % (url, len(issues), len(matchList)))
                                for i in range(len(matchList)):
                                        if i > 0 and not self.__reeditions:
                                                break
                                        self.vprint(1, 'New item from serie %s' % serie)
                                        data = self.__fetchAlbumInfo(matchList[i])
                                        for key, value in EO_data.iteritems():
                                                if len(value):
                                                        data[key] = value
                                        data['bdgest'] = '%s/%s' % (self.__baseURL, url)
                                        #for abr, info in [('TT', 'class-tt'), ('TL', 'class-tl'), ('INT', 'class-int')]:
                                                #if re.search(abr, issue):
                                                        #data[info] = 'true'
                                        for image in ('cover', 'back', 'planche'):
                                                if len(data[image]):
                                                        if self.__image:
                                                                de, vers = {'cover' : ('Couvertures/', 'thb_couv/'), 'back' : ('Versos/', 'thb_versos/'), 'planche' : ('Planches/', 'thb_planches/')}[image]
                                                                url_image = re.sub(de, vers, data[image].split(';')[0])
                                                        else:
                                                                url_image = data[image].split(';')[0]
                                                        data[image] = self.__getImage('%s/%s' % (self.__baseURL, url_image), data['title'])
                                        if (i==0):
                                                if self.__chro:
                                                        # Fetching chronicles URLs of the album
                                                        self.__getChronicleList(serie)
                                                        PO = re.compile(title, 18)
                                                        self.vprint(1, "Looking for critic for %s" % title)
                                                        for chro_url, chron in self.__chronicles.iteritems():
                                                                self.vprint(1, "\tCritic : %s" % chron['title'])
                                                                if PO.search(chron['title']):
                                                                        self.vprint(1, "Trying to acces matching critic for %s" % title)
                                                                        if 'texte' in chron.keys():
                                                                                data['texte_chronique'] = chron['texte']
                                                                        else:
                                                                                self.__data = self.getURL.getHTMLContent('%s%s' % (self.__bdgest, chro_url), 1)
                                                                                match = self.__chronPO['texte'].search(re.sub('\n','',self.__data))
                                                                                if match:
                                                                                        data['texte_chronique'] = "<br/><b>Critique : %s</b>%s" % (match.group('who'), match.group('text'))
                                                                                        self.__chronicles[chro_url]['texte'] = data['texte_chronique']
                                                                                        self.vprint(1, 'Critic found for %s' % data['title'])
                                                                                else:
                                                                                        self.__chronicles[chro_url]['texte'] = ''
                                                                                        self.vprint(1, 'Failed to get critic for %s' % data['title'])
                                                                                break
                                                for key, value in data.iteritems():
                                                        if key in  ['issue', 'title', 'serie', 'texte_chronique', 'publisher', 'language', 'dessin', 'writer', 'coloriste', 'isbn', 'pages', 'note'] and len(value):
                                                                EO_data[key] = value
                                        else:
                                                for key, value in EO_data.iteritems():
                                                        if not len(value):
                                                                if key in ['publisher', 'language', 'dessin', 'writer', 'coloriste', 'isbn', 'pages', 'note']:
                                                                        #Any of these data not available for reeditions will be replaced by the value of the original
                                                                        data[key] = EO_data[key]
                                                        if key in ['issue', 'title', 'serie', 'texte_chronique']:
                                                                #The data for reeditions is replaced by the value of the original
                                                                data[key] = EO_data[key]
                                        self.vprint(1, '1 entry found')
                                        self.__domTree.addEntry(data)
                                        nbr += 1
                for mode, access in self.getURL.URLfrom.iteritems():
                        self.vprint(1, "%s page(s) obtained from %s \t(and %s failures)" % (access[1], mode, access[0]))
                return nbr

        def isbn_cmp(self, isbn1, isbn2):
                isbn1, isbn2 = re.sub('[- ]','', isbn1), re.sub('[- ]','', isbn2)
                return re.search(isbn1, isbn2) or re.search(isbn2, isbn1)

        def dl_cmp(self, dl, data):
                if 'mois' not in data.keys() or 'year' not in data.keys():
                        return 1
                else:
                        return re.match(self.__DL2, '%s/%s' % (data['mois'], data['year'])) or re.match(self.__DL2, data['depotlegal'])

        def run(self, argv):
                # Formatting arguments
                self.__n_args = 0
                arglist = ''
                for args in argv:
                        arglist = '%s %s' % (arglist, args)
                arglist = unicode(arglist, local).encode('UTF-8')
                # Parsing arguments
                args = self.__options
                tmp = re.split(' (?=-[a-z]+)', arglist)
                for argu in tmp[1:]:
                        arg = re.search('(-[a-z]+) ?(.*)', argu)
                        opt, value = arg.group(1), arg.group(2)
                        value = value.strip()
                        if opt not in ['-q', '-v', '-r', '-l', '-i', '-c', '-f']:
                                if len(value) > 0:
                                        args[opt] = value
                                        self.__n_args += 1
                        else:
                                if len(value) > 0:
                                        args[opt] = int(value)
                                else:
                                        if opt == '-c':
                                                args[opt] = 0
                                        else:
                                                args[opt] = 1
                self.__verbose, self.__quiet, self.__reeditions, self.__limit, self.__image, self.__chro = args['-v'], args['-q'], args['-r'], args['-l'], args['-i'], args['-c']
                self.vprint(1, 'Parameters :')
                s = ''
                for key, arg in args.iteritems():
                        if key in ('-isbn', '-t', '-a', '-dl', '-lk'):
                                if key == '-a':
                                        tmp = re.split('[, ]*', arg.strip())
                                        args[key] = tmp[0]
                                elif key == '-dl':
                                        arg = arg.strip()
                                        if re.search('[0-9]{4}/[0-9]{2}', arg):
                                                args[key] = '%s/%s' % (arg[5:7], arg[0:4])
                                elif key == '-t':
                                        tmp = re.match('[A-Z]*[0-9]*\. *(.+)', arg.strip())
                                        if tmp:
                                                args[key] = tmp.group(1)
                                        else:
                                                args[key]= arg
                                        if re.search(';', args[key]):
                                                arg = args[key].split(';')
                                                if len(arg[0]) == 0:
                                                        args[key] = arg[1]
                                                elif len(arg[1]) > 0:
                                                        for notitle in ('Volume', 'Tome'):
                                                                if re.match(notitle, arg[0]):
                                                                        args[key] = arg[1]
                                                else:
                                                        args[key] = ''
                                else:
                                        args[key] = arg.strip()
                                self.vprint(1, '%s      : %s' % (key, args[key]))
                        else:
                                s = '%s\t%s = %s;' % (s, key[1:], args[key])
                self.vprint(1, s)
                self.__isbn, self.__title, self.__author, self.__DL, self.__fast, self.__directlink = args['-isbn'], args['-t'], args['-a'],  args['-dl'], args['-f'], args['-lk']
                self.__isbn2, self.__DL2 = self.__isbn, self.__DL
                if self.__n_args > 0:
                        self.getURL = self.HTMLrequest(self.__verbose)
                        nbr = self.__getAlbums()
                        self.vprint(1, '%s album%s found' % (('No', '') if nbr == 0 else (nbr, '') if nbr <= 1 else (nbr, 's')))
                        if nbr == 0 and (len(self.__isbn) or len(self.__DL)):
                                self.vprint(1, 'No album was found from ISBN and DL, now trying without it')
                                self.__isbn, self.__DL, self.__reeditions = '', '', 1
                                nbr = self.__getAlbums()
                                self.vprint(1, '%s album%s found' % (('Still no', '') if nbr == 0 else (nbr, '') if nbr <= 1 else (nbr, 's')))
                        if nbr > 0:
                                if self.__quiet in [0, 2]:
                                        self.__domTree.printXMLTree()
                else:
                        self.vprint(1, 'No arguments found')
                        return False


        def __getImage(self, url, Id):
                if self.__noimage:
                        return ''
                self.vprint(1, "Downloading image...")
                # Returns the image base64-encoded
                md5 = genMD5()
                img, image = '', ''
                request = urllib2.Request(url.strip())
                request.add_header('User-Agent', 'Mozilla/4.0  (compatible; MSIE 5.5; Windows NT)')
                request.add_header('Referer', '')
                try:
                        imObj = urllib2.urlopen(request)
                        img = imObj.read()
                        imObj.close()
                except:
                        self.vprint(1, 'The image of the album %s does not exist : %s' % (Id, url.strip()))
                        pass
                imgPath = "/tmp/%s.jpeg" % md5
                try:
                        f = open(imgPath, 'w')
                        f.write(img)
                        f.close()
                except:
                        self.vprint(1, 'File protection error : unable to write temporary file ' % imgPath)
                        pass
                if len(img) > 0:
                        image = (md5 + '.jpeg', base64.encodestring(img))
                else:
                        image = ''
                # Delete temporary image
                try:
                        os.remove(imgPath)
                except:
                        self.vprint(1, 'Unable to delete temporary file %s' % imgPath)
                        pass
                self.vprint(2, "...done")
                return image

        def vprint(self, v, st):
                if (v <= self.__verbose):
                        if self.__quiet:
                                try:
                                        print unicode(st, "UTF-8").encode(local)
                                except:
                                        print st
                        else:
                                print '<!--     %s      -->' % st
                        self.__comment = '%s<br \>%s' % (self.__comment, st)
                        return True
                else:
                        return False

def halt():
        sys.exit(0)

def showUsage():
        help = """
Tellico data source setup:
- source name: bedetheque.com (or whatever you want :)
- Collection type: comics collection
- Result type: tellico
- Path: /path/to/script/bedetheque.com.py
- Arguments:
Title   : -t %1
Author  : -a %1
ISBN    : -isbn %1
Keyword : -dl %1
Update  : -t %{title} -isbn %{isbn} -a %{artist} -lk %{lien-bel}

Notes :
- Title stands for album or serie..
- Keyword stands here for legal deposit (dépot légal). it should be entered as 'MM/YYYY' or 'YYYY/MM':
        "-dl 01/2009"
- Author field values must contain name only and not the surname so as to comply with bedetheque.com own search feature. If two separated words are given, either separetad by space or comma, only the first one will be used.

Any other parameter can be added either in the configuration window of the script, or directly in the internet search window. For instance, the following queries are strictly identical :
        " -a sfar -t donjon crépuscule" as Author, Title, ISBN or Keyword
        "sfar -t donjon crépuscule" as Author
        "donjon crépuscule -a sfar" as Title

Optonal arguments :
        -r : tells the script to include reeditions in the search. Default is not to include them.
        -l nbr : Gives a limit in terms of number of albums found to determine wether the images will be downloaded. Default is no-limit (-1). Not downloading images (either big or small) results in significant increase of speed, whatever your bandwidth.
        -i : Only thumbnailed images are downloaded, instead of full size images.
        -c : Go and fetch critics (default is 1). Use it to avoid fetching them.
        -f      [nbr] : fast search will only display serie, title and legal deposit. If nbr is specified, the fast method will be used only if the number of albums (withot reedtions) exceeds nbr.
        -lk URL : given the direct link to the desired page, other parameters (like author or title) are overidden
"""
        try:
                print unicode(help, "UTF-8").encode(local)
        except:
                print help
        sys.exit(1)

def main():
        if len(sys.argv) < 2:
                showUsage()
        parser = BDthequeParser()
        parser.run(sys.argv[1:])

if __name__ == '__main__':
        main()