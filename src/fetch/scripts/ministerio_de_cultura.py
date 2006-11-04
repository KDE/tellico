#!/usr/bin/env python2.4
# -*- coding: iso-8859-1 -*-

# ***************************************************************************
#    copyright            : (C) 2006 by Mathias Monnerville
#    email                : tellico@monnerville.com
# ***************************************************************************
#
# ***************************************************************************
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of version 2 of the GNU General Public License as  *
# *   published by the Free Software Foundation;                            *
# *                                                                         *
# ***************************************************************************

# $Id: books_ministerio_de_cultura.py 242 2006-06-06 11:23:54Z mathias $

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

XML_HEADER = """<?xml version="1.0" encoding="UTF-8"?>"""
DOCTYPE = """<!DOCTYPE tellico PUBLIC "-//Robby Stephenson/DTD Tellico V9.0//EN" "http://periapsis.org/tellico/dtd/v9/tellico.dtd">"""
NULLSTRING = ''

VERSION = "0.2"

ISBN, AUTHOR, TITLE = range(3)

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
		
		# Add a custom 'Collection' field
		self.__customField = self.__doc.createElement('field')                                                                   
		self.__customField.setAttribute('name', 'book_collection')                                                                          
		self.__customField.setAttribute('title', 'Collection')                                                                   
		self.__customField.setAttribute('flags', '7')                                                                            
		self.__customField.setAttribute('category', 'Classification')                                                            
		self.__customField.setAttribute('format', '0')                                                                           
		self.__customField.setAttribute('type', '1')                                                                             
		self.__customField.setAttribute('i18n', 'yes')                                                                           

		self.__fields.appendChild(self.__dfltField)                                                                            
		self.__fields.appendChild(self.__customField)                                                                            
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
		isbnNode.appendChild(self.__doc.createTextNode(d['isbn'][0]))

		priceNode = self.__doc.createElement('pur_price')
		priceNode.appendChild(self.__doc.createTextNode(d['pur_price']))

		bookColNode = self.__doc.createElement('book_collection')
		bookColNode.appendChild(self.__doc.createTextNode(d['collection']))

		for name in (	'title', 'year', 'pub', 'langs', 'keyword', 'ed', 'writers', 
						'comments', 'pages', 'isbn', 'price', 'bookCol' ):
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
		# Search form is at http://www.mcu.es/bases/spa/isbn/ISBN.html
		self.__baseURL 	 = 'http://www.mcu.es'
		self.__searchURL = '/cgi-bin/BRSCGI?CMD=VERLST&BASE=ISBN&CONF=AEISPA.cnf&OPDEF=AND&DOCS=1-10&SEPARADOR=&WGEN-C=&' + \
						   'WISB-C=%s&WAUT-C=%s&WTIT-C=%s&WMAT-C=&WEDI-C=&'
		self.__suffixURL = 'WFEP-C=&%40T353-GE=&%40T353-LE=&WSER-C=&WLUG-C=&WDIS-C=DISPONIBLE+or+AGOTADO&WLEN-C=&WCLA-C=&WSOP-C='

		# Define some regexps
		self.__regExps = { 	'author'		: '<th scope="row">Autor:.*?<ul>(?P<author>.*?)</ul>',
							'isbn'			: '<th scope="row">ISBN:.*?<td>(?P<isbn>.*?)</td>',
							'title'			: '<th scope="row">T.tulo:.*?<td>(?P<title>.*?)</td>',
							'language'		: '<th scope="row">Lengua:.*?<li>(?P<language>.*?)</li>',
							'edition'		: '<th scope="row">Edici.n:.*?<td>(?P<edition>.*?)</td>',
							'pur_price'		: '<th scope="row">Precio:.*?<li>(?P<pur_price>.*?)</li>',
							'desc'			: '<th scope="row">Descripci.n:.*?<td>(?P<desc>.*?)</td>',
							'publication'	: '<th scope="row">Publicaci.n:.*?<li>(?P<publication>.*?)</li>',
							'collection'	: '<th scope="row">Colecci.n:.*?<li>(?P<collection>.*?)</li>',
							'keyword'		: '<th scope="row">Materias:.*?<li>(?P<keywords>.*?)</li>',
							'cdu'			: '<th scope="row">CDU:.*?<td>(?P<cdu>.*?)</td>',
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
			criteria = criteria.replace('-', '')
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
		u = urllib2.urlopen(url)
		self.__data = u.read()
		u.close()

	def __fetchBookLinks(self):
		"""
		Retrieve all links related to the search. self.__data contains HTML content fetched by self.__getHTMLContent() 
		that need to be parsed.
		"""
		matchList = re.findall("""<div class="info"><p>.*?<A target="_top" HREF="(?P<url>.*?)">""", self.__data, re.S)

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

		for name, po in self.__regExpsPO.iteritems():
			data[name] = NULLSTRING
			matches[name] = re.search(self.__regExps[name], self.__data, re.S | re.I)

			if matches[name]:
				if name == 'title':
					data['title'] = matches[name].group('title').strip()

				elif name == 'isbn':
					d = matches[name].group('isbn').strip()
					d = re.sub('&nbsp;', '', d)
					d = re.sub('</?[Aa].*?>', '', d)
					isbnList = re.findall('[\w\d-]+', d)
				
					# Only first element will be added to the ibsn element
					data['isbn'] = isbnList

				elif name == 'edition':
					data['edition'] = matches[name].group('edition').strip()

				elif name == 'pur_price':
					d = matches[name].group('pur_price')
					d = re.sub('</?span.*?>', '', d)
					d = re.sub('\n\x80', '', d)
					data['pur_price'] = d.strip() + ' EUR'

				elif name == 'publication':
					d = matches[name].group('publication')
					for p in ('</?[Aa].*?>', '&nbsp;', ':', ','):
						d = re.sub(p, '', d)

					d = d.split('\n')
					# d[1] is an empty string
					data['publisher'] = "%s (%s)" % (d[2], d[0])
					data['pub_year'] = re.sub('\d{2}\/', '', d[3])

				elif name == 'desc':
					d = matches[name].group('desc')
					m = re.search('\d+ ', d)
					data['pages'] = m.group(0).strip()
					m = re.search('; (?P<format>.*cm)', d)
					if m:
						data['comments'].append('Format: ' + m.group('format').strip())
					del data['desc']

				elif name == 'collection':
					d = matches[name].group('collection').strip()
					d = re.sub('&nbsp;', '', d)
					data[name] = d

				elif name == 'keyword':
					d = matches[name].group('keywords')
					d = re.sub('</?[Aa].*?>', '', d)
					data['keyword'] = d.strip()

				elif name == 'cdu':
					data['comments'].append('CDU: ' + matches[name].group('cdu').strip())

				elif name == 'author':
					# We may find several authors
					data[name] = []
					authorsList = re.findall('<a.*?>(?P<author>.*?)</a>', matches[name].group('author'), re.S | re.I)
					if not authorsList:
						# No href links
						authors = re.search('<li>(?P<author>.*?)</li>', matches[name].group('author'), re.S | re.I)
						results = authors.group('author').strip().split(',')
						results = [r.strip() for r in results]
						data[name] = results
					else:
						for d in authorsList:
							# Sometimes, the search engine outputs some image between a elements
							if d.strip()[:4] != '<img':
								data[name].append(d.strip())

				elif name == 'language':
					# We may find several languages
					d =  matches[name].group('language')
					d = re.sub('</?span.*?>', '', d)
					d = re.sub('&nbsp;', '', d)
					d = d.strip()
					d = d.split('\n')
					a = []
					i = 0
					while i < len(d):
						if d[i][-1:] == ':':
							a.append("%s %s" % (d[i], d[i+1]))
						i += 1
					data['language'] = a	

		return data


	def __getBook(self, data, kind = ISBN):
		if not len(data): 
			raise EngineError, "No data given. Unable to proceed."

		if kind == ISBN:
			self.__getHTMLContent("%s%s%s" % (self.__baseURL, self.__searchURL % \
				(urllib.quote(data),		# ISBN
				 '',						# AUTHOR
				 ''),						# TITLE
				 self.__suffixURL)
				)
		elif kind == AUTHOR:
			self.__getHTMLContent("%s%s%s" % (self.__baseURL, self.__searchURL % \
				('',						# ISBN
				 urllib.quote(data),		# AUTHOR
				 ''),						# TITLE
				 self.__suffixURL)
				)

		elif kind == TITLE:
			self.__getHTMLContent("%s%s%s" % (self.__baseURL, self.__searchURL % \
				('',						# ISBN
				 '',						# AUTHOR
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
				# Print entries on-the-fly
				#self.__domTree.printEntry(node)
		else:
			return None

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

	# ;-separated ISBNs string
	isbnStringList = ''

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
