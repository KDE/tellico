#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-

# ***************************************************************************
#    Copyright (C) 2007-2009 Mathias Monnerville <tellico@monnerville.com>
# ***************************************************************************
#
# ***************************************************************************
# *                                                                         *
# *   This program is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU General Public License as        *
# *   published by the Free Software Foundation; either version 2 of        *
# *   the License or (at your option) version 3 or any later version        *
# *   accepted by the membership of KDE e.V. (or its successor approved     *
# *   by the membership of KDE e.V.), which shall act as a proxy            *
# *   defined in Section 14 of version 3 of the license.                    *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU General Public License for more details.                          *
# *                                                                         *
# *   You should have received a copy of the GNU General Public License     *
# *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
# *                                                                         *
# ***************************************************************************

# $Id: books_ministerio_de_cultura.py 428 2007-03-07 13:17:17Z mathias $

"""
This script has to be used with tellico (http://periapsis.org/tellico) as an external data source program.
It allows searching for books in Spanish Ministry of Culture's database (at http://www.mcu.es/bases/spa/isbn/ISBN.html).

Multiple ISBN/UPC searching is supported through the -m option:
	./books_ministerio_de_cultura.py -m filename
where filename holds one ISBN or UPC per line.

Tellico data source setup:
- Source type: External Application
- Source name: Ministerio de Cultura (ES) (or whatever you want :)
- Collection type: Book Collection
- Result type: Tellico
- Path: /path/to/script/books_ministerio_de_cultura.py
- Arguments:
Title (checked) 	= -t %1
Person (checked) 	= -a %1
ISBN (checked)  	= -i %1
UPC (checked)		= -i %1
Update (checked) = %{title}

** Please note that this script is also part of the Tellico's distribution. 
** You will always find the latest version in the SVN trunk of Tellico

SVN Version:	
	* Removes translators for Authors List
	* Adds translators to translator field
	* Change from "Collection" to "Series"
	* Process "Series Number"
	* Adds in comments "ed.lit." authors
	* If there isn't connection to Spanish Ministry of Culture
	  shows a nice error message (timeout: 5 seconds)
	* Removed "translated from/to" from Comments field as already
	  exists in "Publishing" field
	* Removed "Collection" field as I moved to Series/Series Number

Version 0.3.2:
	* Now find 'notas' field related information
	* search URL modified to fetch information of exhausted books too

Version 0.3.1:
Bug Fixes:
	* The 'tr.' string does not appear among authors anymore
	* Fixed an AttributeError exception related to a regexp matching the number of pages

Version 0.3:
Bug Fixes:
	* URL of the search engine has changed:
		http://www.mcu.es/bases/spa/isbn/ISBN.html is now http://www.mcu.es/comun/bases/isbn/ISBN.html
	* All the regexps have been rewritten to match the new site's content

Version 0.2:
New features:
	* Support for multiple ISBN/UPC searching (support from command line with -m option)
	* Default books collection enhanced with a new custom field 'Collection'
	* Search extended for both available and exhausted books
	* Hyphens are stripped out in the ISBN (or UPC) search

Bug Fixes:
	* Publication year now holds only the year
	* ISBN regexp fix
	* Fix for publisher field (values were inverted)
	* -i parameter works for both ISBN and UPC based search

Version 0.1:
	* Initial Release
"""

import sys, os, re, md5, random, string
import urllib, urllib2, time, base64
import xml.dom.minidom, types
import socket

XML_HEADER = """<?xml version="1.0" encoding="UTF-8"?>"""
DOCTYPE = """<!DOCTYPE tellico PUBLIC "-//Robby Stephenson/DTD Tellico V9.0//EN" "http://periapsis.org/tellico/dtd/v9/tellico.dtd">"""
NULLSTRING = ''

VERSION = "0.3.2"

ISBN, AUTHOR, TITLE = range(3)

TRANSLATOR_STR = "tr."
EDLIT_STR = "ed. lit."

class EngineError(Exception): pass

class BasicTellicoDOM:
	"""
	This class manages tellico's XML data model (DOM)
	"""
	def __init__(self):
		self.__doc = xml.dom.minidom.Document()
		self.__root = self.__doc.createElement('tellico')
		self.__root.setAttribute('xmlns', 'http://periapsis.org/tellico/')
		self.__root.setAttribute('syntaxVersion', '9')
		
		self.__collection = self.__doc.createElement('collection')
		self.__collection.setAttribute('title', 'My Books')
		self.__collection.setAttribute('type', '2')

		self.__fields = self.__doc.createElement('fields')                                                                  
		# Add all default (standard) fields
		self.__dfltField = self.__doc.createElement('field')                                                                   
		self.__dfltField.setAttribute('name', '_default')                                                                      
		
		# Add a custom 'Collection' field (Left by reference for
		# the future)
		#self.__customCollectionField = self.__doc.createElement('field')
		#self.__customCollectionField.setAttribute('name', 'book_collection')
		#self.__customCollectionField.setAttribute('title', 'Collection')
		#self.__customCollectionField.setAttribute('flags', '7')
		#self.__customCollectionField.setAttribute('category', 'Classification')
		#self.__customCollectionField.setAttribute('format', '0')
		#self.__customCollectionField.setAttribute('type', '1')
		#self.__customCollectionField.setAttribute('i18n', 'yes')


		self.__fields.appendChild(self.__dfltField)
		#self.__fields.appendChild(self.__customCollectionField)
		self.__collection.appendChild(self.__fields)

		self.__root.appendChild(self.__collection)
		self.__doc.appendChild(self.__root)

		# Current movie id. See entry's id attribute in self.addEntry()
		self.__currentId = 0


	def addEntry(self, movieData):
		"""
		Add a comic entry. 
		Returns an entry node instance
		"""

		d = movieData

		# Convert all strings to UTF-8
		for i in d.keys():
			if type(d[i]) == types.ListType:
				d[i] = [unicode(d[i][j], 'latin-1').encode('utf-8') for j in range(len(d[i]))]
			elif type(d[i]) == types.StringType:
				d[i] = unicode(d[i], 'latin-1').encode('utf-8')

		entryNode = self.__doc.createElement('entry')
		entryNode.setAttribute('id', str(self.__currentId))

		titleNode = self.__doc.createElement('title')
		titleNode.appendChild(self.__doc.createTextNode(d['title']))

		yearNode = self.__doc.createElement('pub_year')
		yearNode.appendChild(self.__doc.createTextNode(d['pub_year']))

		pubNode = self.__doc.createElement('publisher')
		pubNode.appendChild(self.__doc.createTextNode(d['publisher']))

		langsNode = self.__doc.createElement('languages')
		for l in d['language']:
			langNode = self.__doc.createElement('language')
			langNode.appendChild(self.__doc.createTextNode(l))
			langsNode.appendChild(langNode)

		keywordsNode = self.__doc.createElement('keywords')
		keywordNode = self.__doc.createElement('keyword')
		keywordNode.appendChild(self.__doc.createTextNode(d['keyword']))
		keywordsNode.appendChild(keywordNode)

		edNode = self.__doc.createElement('edition')
		edNode.appendChild(self.__doc.createTextNode(d['edition']))

		writersNode = self.__doc.createElement('authors')
		for g in d['author']:
			writerNode = self.__doc.createElement('author')
			writerNode.appendChild(self.__doc.createTextNode(g))
			writersNode.appendChild(writerNode)

		commentsNode = self.__doc.createElement('comments')
		commentsData = string.join(d['comments'], '<br/>')
		commentsNode.appendChild(self.__doc.createTextNode(commentsData))

		pagesNode = self.__doc.createElement('pages')
		pagesNode.appendChild(self.__doc.createTextNode(d['pages']))

		isbnNode = self.__doc.createElement('isbn')
		isbnNode.appendChild(self.__doc.createTextNode(d['isbn']))

		priceNode = self.__doc.createElement('pur_price')
		priceNode.appendChild(self.__doc.createTextNode(d['pur_price']))

		seriesNode = self.__doc.createElement('series')
		seriesNode.appendChild(self.__doc.createTextNode(d['series']))

		seriesNumNode = self.__doc.createElement('series_num')
		seriesNumNode.appendChild(self.__doc.createTextNode(d['series_num']))

		translatorNode = self.__doc.createElement('translator')
		translatorNode.appendChild(self.__doc.createTextNode(d['translator']))

		for name in ( 'title', 'year', 'pub', 'langs', 'keyword', 'ed', 'writers', 
			'comments', 'pages', 'isbn', 'price', 'series', 'seriesNum', 'translator' ):
			entryNode.appendChild(eval(name + 'Node'))

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

		print XML_HEADER; print DOCTYPE
		print self.__root.toxml()


class MinisterioCulturaParser:
	def __init__(self):
		# Search form is at http://www.mcu.es/comun/bases/isbn/ISBN.html
		self.__baseURL	 = 'http://www.mcu.es'
		self.__searchURL = '/cgi-brs/BasesHTML/isbn/BRSCGI?CMD=VERLST&BASE=ISBN&DOCS=1-15&CONF=AEISPA.cnf&OPDEF=AND&SEPARADOR=' + \
						   '&WDIS-C=DISPONIBLE+or+AGOTADO&WGEN-C=&WISB-C=%s&WAUT-C=%s&WTIT-C=%s&WMAT-C=&WEDI-C=&'

		self.__suffixURL = 'WFEP-C=&%40T353-GE=&%40T353-LE=&WSER-C=&WLUG-C=&WLEN-C=&WCLA-C=&WSOP-C='

		# Define some regexps
		self.__regExps = { 	'author'		: '<th scope="row">Autor:.*?<td>(?P<author>.*?)</td>',
							'isbn'			: '<span class="cabTitulo">ISBN.*?<strong>(?P<isbn>.*?)</strong>',	# Matches ISBN 13
							'title'			: '<th scope="row">T&iacute;tulo:.*?<td>(?P<title>.*?)</td>',
							'language'		: '<th scope="row">Lengua:.*?<td>(?P<language>.*?)</td>',
							'edition'		: '<th scope="row">Edici&oacute;n:.*?<td>.*?<span>(?P<edition>.*?)</span>',
							'pur_price'		: '<th scope="row">Precio:.*?<td>.*?<span>(?P<pur_price>.*?)&euro;</span>',
							'desc'			: '<th scope="row">Descripci&oacute;n:.*?<td>.*?<span>(?P<desc>.*?)</span>',
							'publication'	: '<th scope="row">Publicaci&oacute;n:.*?<td>.*?<span>(?P<publication>.*?)</span>',
							'keyword'		: '<th scope="row">Materias:.*?<td>.*?<span>(?P<keywords>.*?)</span>',
							'notas'			: '<th scope="row">Notas:.*?<td>.*?<span>(?P<notas>.*?)</span>',
							'cdu'			: '<th scope="row">CDU:.*?<td><span>(?P<cdu>.*?)</span></td>',
							'encuadernacion': '<th scope="row">Encuadernaci&oacute;n:.*?<td>.*?<span>(?P<encuadernacion>.*?)</span>',
							'series'	: '<th scope="row">Colecci&oacute;n:.*?<td>.*?<span>(?P<series>.*?)</span>'
						}	

		# Compile patterns objects
		self.__regExpsPO = {}
		for k, pattern in self.__regExps.iteritems():
			self.__regExpsPO[k] = re.compile(pattern)

		self.__domTree = BasicTellicoDOM()

	def run(self, criteria, kind):
		"""
		Runs the parser: fetch book related links, then fills and prints the DOM tree
		to stdout (in tellico format) so that tellico can use it.
		"""

		# Strip out hyphens if kind is ISBN
		if kind == ISBN:
			criteria = criteria.replace('-', NULLSTRING)
			# Support for multiple search
			isbnList = criteria.split(';')
			for n in isbnList:
				self.__getBook(n, kind)
		else:
			self.__getBook(criteria, kind)

		# Print results to stdout
		self.__domTree.printXMLTree()

	def __getHTMLContent(self, url):
		"""
		Fetch HTML data from url
		"""
		
		try:
		    u = urllib2.urlopen(url)
		except Exception, e:
			u.close()
			sys.exit("""
Network error while getting HTML content.
Tellico cannot connect to: http://www.mcu.es/comun/bases/isbn/ISBN.htm webpage:
'%s'""" % e)


		self.__data = u.read()
		u.close()

	def __fetchBookLinks(self):
		"""
		Retrieve all links related to the search. self.__data contains HTML content fetched by self.__getHTMLContent() 
		that need to be parsed.
		"""

		matchList = re.findall("""<div class="isbnResDescripcion">.*?<p>.*?<A target="_top" HREF="(?P<url>.*?)">""", self.__data, re.S)

		if not matchList: return None
		return matchList

	def __fetchBookInfo(self, url):
		"""
		Looks for book information
		"""

		self.__getHTMLContent(url)

		matches = {}
		data = {}

		data['comments'] = []
		# Empty string if series not available
		data['series_num'] = NULLSTRING 
		data['translator'] = NULLSTRING

		for name, po in self.__regExpsPO.iteritems():
			data[name] = NULLSTRING
			matches[name] = re.search(self.__regExps[name], self.__data, re.S | re.I)


			if matches[name]:
				if name == 'title':
					d = matches[name].group('title').strip()
					d = re.sub('<.?strong>', NULLSTRING, d)
					d = re.sub('\n', NULLSTRING, d)
					data['title'] = d

				elif name == 'isbn':
					data['isbn'] = matches[name].group('isbn').strip()

				elif name == 'edition':
					data['edition'] = matches[name].group('edition').strip()

				elif name == 'pur_price':
					d = matches[name].group('pur_price')
					data['pur_price'] = d.strip() + ' EUR'

				elif name == 'publication':
					d = matches[name].group('publication')
					for p in ('</?[Aa].*?>', '&nbsp;', ':', ','):
						d = re.sub(p, NULLSTRING, d)

					d = d.split('\n')
					# d[1] is an empty string
					data['publisher'] = "%s (%s)" % (d[2], d[0])
					data['pub_year'] = re.sub('\d{2}\/', NULLSTRING, d[3])
					del data['publication']

				elif name == 'desc':
					d = matches[name].group('desc')
					m = re.search('\d+ ', d)
					# When not available
					data['pages'] = NULLSTRING
					if m:
						data['pages'] = m.group(0).strip()
					m = re.search('; (?P<format>.*cm)', d)
					if m:
						data['comments'].append('Format: ' + m.group('format').strip())
					del data['desc']

				elif name == 'encuadernacion':
					data['comments'].append(matches[name].group('encuadernacion').strip())

				elif name == 'keyword':
					d = matches[name].group('keywords')
					d = re.sub('</?[Aa].*?>', NULLSTRING, d)
					data['keyword'] = d.strip()

				elif name == 'cdu':
					data['comments'].append('CDU: ' + matches[name].group('cdu').strip())
				
				elif name == 'notas':
					data['comments'].append(matches[name].group('notas').strip())
				
				elif name == 'series':
					d = matches[name].group('series').strip()
					d = re.sub('&nbsp;', ' ', d)
					data[name] = d
					# data[name] can contain something like 'Byblos, 162/24'

					# Maybe better to add the reg exp to get seriesNum in self.__regExps 
					p = re.compile('[0-9]+$')
					s = re.search(p, data[name])

					if s:
						# if series ends with a number, it seems that is a 
						# number of the book inside the series. We save in seriesNum
						data['series_num'] = s.group()

						# it removes lasts digits (plus one because is space or /) from
						# data['series']
						l = len(data['series_num']) + 1
						data[name] = data[name][0:-l]
						data[name] = data[name].rstrip(",") # remove the , between series and series_num

				elif name == 'author':
					# We may find several authors
					data[name] = []
					authorsList = re.findall('<a.*?>(?P<author>.*?)</a>', matches[name].group('author'), re.S | re.I)
					if not authorsList:
						# No href links
						authors = re.search('<li>(?P<author>.*?)</li>', matches[name].group('author'), re.S | re.I)
						try:
							results = authors.group('author').strip().split(',')
						except AttributeError:
							results = []
						results = [r.strip() for r in results]
						data[name] = results
					else:
						for d in authorsList:
							# Sometimes, the search engine outputs some image between a elements
							if d.strip()[:4] != '<img':
								data[name].append(d.strip())
					
					# Move tr authors (translators) to translators list
					translator = self.__getSpecialRol(data[name], TRANSLATOR_STR)
					edlit = self.__getSpecialRol(data[name], EDLIT_STR)
					data[name] = self.__removeSpecialsFromAuthors(data[name], translator, TRANSLATOR_STR)
					data[name] = self.__removeSpecialsFromAuthors(data[name], edlit, EDLIT_STR)

					if len(translator) > 0:
						data['translator'] = self.__formatSpecials(translator, NULLSTRING)

					if len(edlit) > 0:
						data['comments'].append(self.__formatSpecials(edlit, "Editor Literario: "))

				elif name == 'language':
					# We may find several languages
					d =  matches[name].group('language')
					d = re.sub('\n', NULLSTRING, d)
					d = d.split('<span>')
					a = []
					for lg in d:
						if len(lg):
							lg = re.sub('</span>', NULLSTRING, lg)
							# Because HTML is not interpreted in the 'language' field of Tellico
							lg = re.sub('&oacute;', 'o', lg)
							a.append(lg.strip())
					# Removes that word so that only the language name remains.
					a[0] = re.sub('publicacion: ', NULLSTRING, a[0])
					data['language'] = a
					# Add other language related info to the 'comments' field too
					#for lg in a[1:]:
						#data['comments'].append(lg)

		return data


	def __getBook(self, data, kind = ISBN):
		if not len(data): 
			raise EngineError, "No data given. Unable to proceed."

		if kind == ISBN:
			self.__getHTMLContent("%s%s%s" % (self.__baseURL, self.__searchURL % \
				(urllib.quote(data),		# ISBN
				 NULLSTRING,				# AUTHOR
				 NULLSTRING),				# TITLE
				 self.__suffixURL)
				)
		elif kind == AUTHOR:
			self.__getHTMLContent("%s%s%s" % (self.__baseURL, self.__searchURL % \
				(NULLSTRING,				# ISBN
				 urllib.quote(data),		# AUTHOR
				 NULLSTRING),				# TITLE
				 self.__suffixURL)
				)

		elif kind == TITLE:
			self.__getHTMLContent("%s%s%s" % (self.__baseURL, self.__searchURL % \
				(NULLSTRING,				# ISBN
				 NULLSTRING,				# AUTHOR
				 urllib.quote(data)),		# TITLE
				 self.__suffixURL)
				)

		# Get all links
		links = self.__fetchBookLinks()

		# Now retrieve infos
		if links:
			for entry in links:
				data = self.__fetchBookInfo( url = self.__baseURL + entry.replace(' ', '%20') )
				node = self.__domTree.addEntry(data)
		else:
			return None

	def __getSpecialRol(self, authors, special):
		"""
		Receives a list like ['Stephen King','Lorenzo Cortina','tr.',
		'Rosalía Vázquez','tr.'] and returns a list with special names
		"""

		j = 0; max = len(authors)
		special_rol = []
		while j < max:
			if authors[j] == special:
				special_rol.append(authors[j-1])
			j += 1
		
		return special_rol

	def __removeSpecialsFromAuthors(self, authors, specials, string):
		"""
		Receives a list with authors+translators and removes 'tr.' and 
		authors from there. Example:
		authors: ['Stephen King','Lorenzo Cortina','tr.','Rosalía Vázquez','tr.']
		translators: ['Lorenzo Cortina','Rosalía Vázquez']
		returns: ['Stephen King']

		(We could also guess string value because is the next position
		in authors list)
		"""

		newauthors = authors[:]

		for t in specials:
			newauthors.remove(t)
			newauthors.remove(string)

		return newauthors

	def __formatSpecials(self, translators, prefix):
		"""
		Receives a list with translators and returns a string
		(authors are handled different: each author in a different node)
		"""

		return prefix + string.join(translators, '; ')

def halt():
	print "HALT."
	sys.exit(0)

def showUsage():
	print """Usage: %s options
Where options are:
  -t  title
  -i  (ISBN|UPC)
  -a  author
  -m  filename   (support for multiple ISBN/UPC search)""" % sys.argv[0]
	sys.exit(1)

def main():
	if len(sys.argv) < 3:
		showUsage()

	socket.setdefaulttimeout(5)

	# ;-separated ISBNs string
	isbnStringList = NULLSTRING

	opts = {'-t' : TITLE, '-i' : ISBN, '-a' : AUTHOR, '-m' : isbnStringList}
	if sys.argv[1] not in opts.keys():
		showUsage()

	if sys.argv[1] == '-m':
		try:
			f = open(sys.argv[2], 'r')
			data = f.readlines()
			# remove trailing \n
			sys.argv[2] = string.join([d[:-1] for d in data], ';')
			sys.argv[1] = '-i'
			f.close()
		except IOError, e:
			print "Error: %s" % e
			sys.exit(1)

	parser = MinisterioCulturaParser()
	parser.run(sys.argv[2], opts[sys.argv[1]])

if __name__ == '__main__':
	main()
