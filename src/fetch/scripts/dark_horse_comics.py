#!/usr/bin/env python
# -*- coding: utf-8 -*-

# ***************************************************************************
#    Copyright (C) 2006-2009 Mathias Monnerville <tellico@monnerville.com>
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

# $Id: comics_darkhorsecomics.py 123 2006-03-24 08:47:48Z mathias $

"""
This script has to be used with tellico as an external data source program.
It allows searching through the Dark Horse Comics web database.

Related info and cover are fetched automatically. It takes only one argument (comic title).

Tellico data source setup:
- source name: Dark Horse Comics (US) (or whatever you want :)
- Collection type: comics collection
- Result type: tellico
- Path: /path/to/script/comics_darkhorsecomics.py
- Arguments:
Title (checked) = %1
Update (checked) = %{title}
"""

import sys, os, re, hashlib, random, string
import urllib, time, base64
import xml.dom.minidom
from urllib.request import urlopen

XML_HEADER = """<?xml version="1.0" encoding="UTF-8"?>"""
DOCTYPE = """<!DOCTYPE tellico PUBLIC "-//Robby Stephenson/DTD Tellico V9.0//EN" "http://periapsis.org/tellico/dtd/v9/tellico.dtd">"""
NULLSTRING = ''

VERSION = "0.2"


def genMD5():
	"""
	Generates and returns a random md5 string. Its main purpose is to allow random
	image file name generation.
	"""
	float = random.random()
	return hashlib.md5(str(float).encode()).hexdigest()

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
		self.__collection.setAttribute('title', 'My Comics')
		self.__collection.setAttribute('type', '6')

		self.__fields = self.__doc.createElement('fields')
		# Add all default (standard) fields
		self.__dfltField = self.__doc.createElement('field')
		self.__dfltField.setAttribute('name', '_default')

		self.__fields.appendChild(self.__dfltField)
		self.__collection.appendChild(self.__fields)

		self.__images = self.__doc.createElement('images')

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
		entryNode = self.__doc.createElement('entry')
		entryNode.setAttribute('id', str(self.__currentId))

		titleNode = self.__doc.createElement('title')
		titleNode.appendChild(self.__doc.createTextNode(d['title']))
		entryNode.appendChild(titleNode)

		yearNode = self.__doc.createElement('pub_year')
		yearNode.appendChild(self.__doc.createTextNode(d['pub_year']))
		entryNode.appendChild(yearNode)

		countryNode = self.__doc.createElement('country')
		countryNode.appendChild(self.__doc.createTextNode(d['country']))
		entryNode.appendChild(countryNode)
		pubNode = self.__doc.createElement('publisher')
		pubNode.appendChild(self.__doc.createTextNode(d['publisher']))
		entryNode.appendChild(pubNode)
		langNode = self.__doc.createElement('language')
		langNode.appendChild(self.__doc.createTextNode(d['language']))
		entryNode.appendChild(langNode)

		writersNode = self.__doc.createElement('writers')
		for g in d['writer']:
			writerNode = self.__doc.createElement('writer')
			writerNode.appendChild(self.__doc.createTextNode(g))
			writersNode.appendChild(writerNode)
		entryNode.appendChild(writersNode)

		genresNode = self.__doc.createElement('genres')
		for g in d['genre']:
			genreNode = self.__doc.createElement('genre')
			genreNode.appendChild(self.__doc.createTextNode(g))
			genresNode.appendChild(genreNode)
		entryNode.appendChild(genresNode)

		commentsNode = self.__doc.createElement('comments')
		#for g in d['comments']:
		#	commentsNode.appendChild(self.__doc.createTextNode(str("%s\n\n" % g, 'latin-1').encode('utf-8')))
		commentsData = '\n\n'.join(d['comments'])
		commentsNode.appendChild(self.__doc.createTextNode(commentsData))
		entryNode.appendChild(commentsNode)

		artistsNode = self.__doc.createElement('artists')
		for k, v in iter(d['artist'].items()):
			if isinstance(v, str) and v != 'various':
                                artistNode = self.__doc.createElement('artist')
                                artistNode.appendChild(self.__doc.createTextNode(v))
                                artistsNode.appendChild(artistNode)
			elif isinstance(v, list):
                                for g in v:
                                        artistNode = self.__doc.createElement('artist')
                                        artistNode.appendChild(self.__doc.createTextNode(g))
                                        artistsNode.appendChild(artistNode)
		entryNode.appendChild(artistsNode)

		if 'pages' in d:
			pagesNode = self.__doc.createElement('pages')
			pagesNode.appendChild(self.__doc.createTextNode(d['pages']))
			entryNode.appendChild(pagesNode)

		if 'isbn' in d:
			isbnNode = self.__doc.createElement('isbn')
			isbnNode.appendChild(self.__doc.createTextNode(d['isbn']))
			entryNode.appendChild(isbnNode)

		if 'issue' in d:
			issueNode = self.__doc.createElement('issue')
			issueNode.appendChild(self.__doc.createTextNode(d['issue']))
			entryNode.appendChild(issueNode)

		if 'image' in d:
			imageNode = self.__doc.createElement('image')
			imageNode.setAttribute('format', 'JPEG')
			imageNode.setAttribute('id', d['image'][0])
			imageNode.appendChild(self.__doc.createTextNode(d['image'][1].decode(encoding='utf-8')))

			coverNode = self.__doc.createElement('cover')
			coverNode.appendChild(self.__doc.createTextNode(d['image'][0]))
			entryNode.appendChild(coverNode)

		if 'image' in d:
			entryNode.appendChild(coverNode)
			self.__images.appendChild(imageNode)

		self.__collection.appendChild(entryNode)

		self.__currentId += 1
		return entryNode

	def printEntry(self, nEntry):
		"""
		Prints entry's XML content to stdout
		"""
		try:
			print(nEntry.toxml())
		except:
			print("Error while outputting XML content from entry to Tellico", file=sys.stderr)

	def printXMLTree(self):
		"""
		Outputs XML content to stdout
		"""
		self.__collection.appendChild(self.__images)
		print(XML_HEADER); print(DOCTYPE)
		print(self.__root.toxml())


class DarkHorseParser:
	def __init__(self):
		self.__baseURL 	 = 'https://www.darkhorse.com'
		self.__basePath  = '/Comics/'
		self.__searchURL = '/Search/%s'
		self.__coverPath = '/covers/'
		self.__movieURL  = self.__baseURL + self.__basePath

		# Define some regexps
		self.__regExps = {
			'title' 	: '<h2 class="title">(?P<title>.*?)</h2>',
			'pub_date'	: '<dt>Pub.* Date:</dt>.*?<dd>(?P<pub_date>.*?)</dd>',
			'isbn'		: '<dt>ISBN-10:</dt><dd>(?P<isbn>.*?)</dd>',
			'desc'		: '<div class="product-description">(?P<desc>.*?)</div>',
			'writer'	: '<dt>Writer: *</dt> *<dd><a.*?>(?P<writer>.*?)</a> *</dd>',
			'cover_artist'	: '<dt>Artist: *</dt> *<dd><a.*?>(?P<cover_artist>.*?)</a> *</dd>',
			'penciller'	: '<dt>Penciller: *</dt> *<dd><a.*>(?P<penciller>.*?)</a> *</dd>',
			'inker'		: '<dt>Inker: *</dt> *<dd><a.*>(?P<inker>.*?)</a> *</dd>',
			'letterer'	: '<dt>Letterer: *</dt> *<dd><a.*>(?P<letterer>.*?)</a> *</dd>',
			'colorist'	: '<dt>Colorist: *</dt> *<dd><a.*>(?P<colorist>.*?)</a> *</dd>',
			'genre'		: '<strong>Genre: *</strong> *<a.*?>(?P<genre>.*?)</a> *</div>',
			'format'	: '<dt>Format: *</dt> *(?P<format>.*?)<dt>',
		}

		# Compile patterns objects
		self.__regExpsPO = {}
		for k, pattern in iter(self.__regExps.items()):
			self.__regExpsPO[k] = re.compile(pattern, re.DOTALL)

		self.__domTree = BasicTellicoDOM()

	def run(self, title):
		"""
		Runs the parser: fetch movie related links, then fills and prints the DOM tree
		to stdout (in tellico format) so that tellico can use it.
		"""
		self.__getMovie(title)
		# Print results to stdout
		self.__domTree.printXMLTree()

	def __getHTMLContent(self, url):
		"""
		Fetch HTML data from url
		"""
		u = urlopen(url)
		# the html says it's in utf-8, but it happens to be latin-1
		self.__data = u.read().decode(encoding='latin-1')
		u.close()

	def __fetchMovieLinks(self):
		"""
		Retrieve all links related to the search. self.__data contains HTML content fetched by self.__getHTMLContent()
		that need to be parsed.
		"""
		matchList = re.findall("""<a href="%s(?P<page>[^"]*?)" class="product_link">.*?</a>""" % self.__basePath.replace('?', '\\?'), self.__data)
		if not matchList: return None

		return list(set(matchList))

	def __fetchCover(self, path):
		"""
		Fetch cover. Returns base64 encoding of data.
		"""
		md5 = genMD5()
		imObj = urlopen(path.strip())
		img = imObj.read()
		imObj.close()
		b64data = (md5 + '.jpeg', base64.b64encode(img))
		return b64data

	def __fetchMovieInfo(self, url):
		"""
		Looks for movie information
		"""
		self.__getHTMLContent(url)

		# First grab picture data
		imgMatch = re.search("""<img src="(?P<imgpath>[^>]*%s.*?)"[^>]*>""" % self.__coverPath, self.__data)
		if imgMatch:
			imgPath = "https:" + imgMatch.group('imgpath')
			# Fetch cover and gets its base64 encoded data
			b64img = self.__fetchCover(imgPath)
		else:
			b64img = None

		# Now isolate data between <div class="bodytext">...</div> elements
		# re.DOTALL makes the "." special character match any character at all, including a newline
		m = re.search("""<div id="inner_content">(?P<part>.*)<div id="right_bar">""", self.__data, re.DOTALL)
		try:
			self.__data = m.group('part')
		except AttributeError:
			self.__data = ""

		matches = {}
		data = {}
		data['comments'] = []
		data['artist'] = {}

		# Default values
		data['publisher'] 	= 'Dark Horse Comics'
		data['language'] 	= 'English'
		data['country'] 	= 'USA'

		if b64img is not None:
			data['image'] 		= b64img
		data['pub_year']	= NULLSTRING

		for name, po in iter(self.__regExpsPO.items()):
			data[name] = NULLSTRING
			if name == 'desc':
				matches[name] = re.findall(self.__regExps[name], self.__data, re.S | re.I)
			else:
				matches[name] = po.search(self.__data)

			if matches[name]:
				if name == 'title':
					title = matches[name].group('title').strip()
					data[name] = title
					# Look for issue information
					m = re.search("#(?P<issue>[0-9]+)", title)
					if m:
						data['issue'] = m.group('issue')
					else:
						data['issue'] = ''

				elif name == 'pub_date':
					pub_date = matches[name].group('pub_date').strip()
					data['pub_year'] = pub_date[-4:]
					# Add this to comments field
					data['comments'].insert(0, "Pub. Date: %s" % pub_date)

				elif name == 'isbn':
					isbn = matches[name].group('isbn').strip()
					data[name] = isbn

				elif name == 'desc':
					# Find biggest size
					max = 0
					for i in range(len(matches[name])):
						if len(matches[name][i]) > len(matches[name][max]):
							max = i
					data['comments'].append(matches[name][max].strip())

				elif name == 'writer':
					# We may find several writers
					data[name] = []
					writersList = re.sub('</?a.*?>', '', matches[name].group('writer')).split(',')
					for d in writersList:
						data[name].append(d.strip())

				elif name == 'cover_artist':
                                        artistsList = matches[name].group('cover_artist').split(',')
                                        data['artist']['Cover Artist'] = []
                                        for d in artistsList:
                                                data['artist']['Cover Artist'].append(d.strip())

				elif name == 'penciller':
					data['artist']['Penciller'] = matches[name].group('penciller').strip()

				elif name == 'inker':
					data['artist']['Inker'] = matches[name].group('inker').strip()

				elif name == 'colorist':
					data['artist']['Colorist'] = matches[name].group('colorist').strip()

				elif name == 'letterer':
					data['artist']['Letterer'] = matches[name].group('letterer').strip()

				elif name == 'genre':
					# We may find several genres
					data[name] = []
					genresList = re.sub('</?a.*?>', '', matches[name].group('genre'), flags=re.DOTALL).split(',')
					for d in genresList:
						data[name].append(d.strip())

				elif name == 'format':
					format = matches[name].group('format').strip()
					data['comments'].insert(1, format)
					m = re.search("(?P<pages>[0-9]+)", format)
					if m:
						data['pages'] = m.group('pages')
					else:
						data['pages'] = ''

		return data


	def __getMovie(self, title):
		if not len(title): return

		self.__title = title
		self.__getHTMLContent("%s%s" % (self.__baseURL, self.__searchURL % urllib.parse.quote(self.__title)))

		# Get all links
		links = self.__fetchMovieLinks()

		# Now retrieve info
		if links:
			for entry in links:
				data = self.__fetchMovieInfo(url = self.__movieURL + entry)
				# Add DC link (custom field)
				data['darkhorse'] = "%s%s" % (self.__movieURL, entry)
				node = self.__domTree.addEntry(data)
				# Print entries on-the-fly
				#self.__domTree.printEntry(node)
		else:
			return None

def halt():
	print("HALT.")
	sys.exit(0)

def showUsage():
	print("Usage: %s comic" % sys.argv[0])
	sys.exit(1)

def main():
	if len(sys.argv) < 2:
		showUsage()

	parser = DarkHorseParser()
	parser.run(sys.argv[1])

if __name__ == '__main__':
	main()
