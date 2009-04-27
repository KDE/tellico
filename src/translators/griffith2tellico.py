#!/usr/bin/env python
# -*- coding: utf-8 -*-
# ***************************************************************************
#    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
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

import os, sys
import base64
import xml.dom.minidom
try:
	import sqlite3
except:
	print sys.stderr, "The Python sqlite3 module is required to import Griffith databases."
	exit(1)

DB_PATH =      os.environ['HOME'] + '/.griffith/griffith.db'
POSTERS_PATH = os.environ['HOME'] + '/.griffith/posters/'

XML_HEADER = """<?xml version="1.0" encoding="UTF-8"?>"""
DOCTYPE = """<!DOCTYPE tellico PUBLIC "-//Robby Stephenson/DTD Tellico V9.0//EN" "http://periapsis.org/tellico/dtd/v9/tellico.dtd">"""

class BasicTellicoDOM:
	def __init__(self):
		self.__doc = xml.dom.minidom.Document()
		self.__root = self.__doc.createElement('tellico')
		self.__root.setAttribute('xmlns', 'http://periapsis.org/tellico/')
		self.__root.setAttribute('syntaxVersion', '9')

		self.__collection = self.__doc.createElement('collection')
		self.__collection.setAttribute('title', 'Griffith Import')
		self.__collection.setAttribute('type', '3')

		self.__fields = self.__doc.createElement('fields')
		# Add all default (standard) fields
		self.__dfltField = self.__doc.createElement('field')
		self.__dfltField.setAttribute('name', '_default')

		# change the rating to have a maximum of 10
		self.__ratingField = self.__doc.createElement('field')
		self.__ratingField.setAttribute('name', 'rating')
		self.__ratingField.setAttribute('title', 'Personal Rating')
		self.__ratingField.setAttribute('flags', '2')
		self.__ratingField.setAttribute('category', 'Personal')
		self.__ratingField.setAttribute('format', '4')
		self.__ratingField.setAttribute('type', '14')
		self.__ratingField.setAttribute('i18n', 'yes')
		propNode = self.__doc.createElement('prop')
		propNode.setAttribute('name', 'maximum')
		propNode.appendChild(self.__doc.createTextNode('10'))
		self.__ratingField.appendChild(propNode);
		propNode = self.__doc.createElement('prop')
		propNode.setAttribute('name', 'minimum')
		propNode.appendChild(self.__doc.createTextNode('1'))
		self.__ratingField.appendChild(propNode);

		# Add a custom 'Original Title' field
		self.__titleField = self.__doc.createElement('field')
		self.__titleField.setAttribute('name', 'orig-title')
		self.__titleField.setAttribute('title', 'Original Title')
		self.__titleField.setAttribute('flags', '8')
		self.__titleField.setAttribute('category', 'General')
		self.__titleField.setAttribute('format', '1')
		self.__titleField.setAttribute('type', '1')
		self.__titleField.setAttribute('i18n', 'yes')

		self.__keywordField = self.__doc.createElement('field')
		self.__keywordField.setAttribute('name', 'keyword')
		self.__keywordField.setAttribute('title', 'Keywords')
		self.__keywordField.setAttribute('flags', '7')
		self.__keywordField.setAttribute('category', 'Personal')
		self.__keywordField.setAttribute('format', '4')
		self.__keywordField.setAttribute('type', '1')
		self.__keywordField.setAttribute('i18n', 'yes')

		self.__urlField = self.__doc.createElement('field')
		self.__urlField.setAttribute('name', 'url')
		self.__urlField.setAttribute('title', 'URL')
		self.__urlField.setAttribute('flags', '0')
		self.__urlField.setAttribute('category', 'General')
		self.__urlField.setAttribute('format', '4')
		self.__urlField.setAttribute('type', '7')
		self.__urlField.setAttribute('i18n', 'yes')

		self.__fields.appendChild(self.__dfltField)
		self.__fields.appendChild(self.__ratingField)
		self.__fields.appendChild(self.__titleField)
		self.__fields.appendChild(self.__keywordField)
		self.__fields.appendChild(self.__urlField)
		self.__collection.appendChild(self.__fields)

		self.__images = self.__doc.createElement('images')

		self.__root.appendChild(self.__collection)
		self.__doc.appendChild(self.__root)
		self.__fieldsMap = dict(country='nationality',
		                     classification='certification',
							 runtime='running-time',
							 o_title='orig-title',
							 notes='comments',
							 image='cover',
							 tag='keyword',
							 site='url')


	def addMedia(self, media):
		if len(media) == 0: return
		# add default Tellico values
		orig_media = 'DVD;VHS;VCD;DivX;Blu-ray;HD DVD'.split(';')
		orig_media.extend(media)
		# make sure unique
		set = {}
		media = [set.setdefault(e,e) for e in orig_media if e not in set]

		mediaField = self.__doc.createElement('field')
		mediaField.setAttribute('name', 'medium')
		mediaField.setAttribute('title', 'Medium')
		mediaField.setAttribute('flags', '2')
		mediaField.setAttribute('category', 'General')
		mediaField.setAttribute('format', '4')
		mediaField.setAttribute('type', '3')
		mediaField.setAttribute('i18n', 'yes')
		mediaField.setAttribute('allowed', ';'.join(media))
		self.__fields.appendChild(mediaField)

	def addEntry(self, movieData):
		"""
		Add a movie entry
		"""
		entryNode = self.__doc.createElement('entry')
		entryNode.setAttribute('id', movieData['id'])

		for key, values in movieData.iteritems():
			if key == 'id':
				continue

			if self.__fieldsMap.has_key(key):
				field = self.__fieldsMap[key]
			else:
				field = key

			parentNode = self.__doc.createElement(field + 's')

			for value in values:
				if len(value) == 0: continue
				node = self.__doc.createElement(field)
				if field == 'certification': value += " (USA)"
				elif field == 'region': value = "Region " + value
				elif field == 'cover':
					imageNode = self.__doc.createElement('image')
					imageNode.setAttribute('format', 'JPEG')
					imageNode.setAttribute('id', value[0])
					imageNode.appendChild(self.__doc.createTextNode(value[1]))
					self.__images.appendChild(imageNode)
					value = value[0] # value was (id, md5)

				if field == 'cast':
					for v in value:
						columnNode = self.__doc.createElement('column')
						columnNode.appendChild(self.__doc.createTextNode(v.strip()))
						node.appendChild(columnNode)

				else:
					node.appendChild(self.__doc.createTextNode(value.strip()))

				if node.hasChildNodes(): parentNode.appendChild(node)

			if parentNode.hasChildNodes(): entryNode.appendChild(parentNode)

		self.__collection.appendChild(entryNode)

	def printXML(self):
		"""
		Outputs XML content to stdout
		"""
		self.__collection.appendChild(self.__images)
		print XML_HEADER; print DOCTYPE
		print self.__root.toxml()


class GriffithParser:
	def __init__(self):
		self.__dbPath = DB_PATH
		self.__domTree = BasicTellicoDOM()

	def run(self):
		"""
		Runs the parser: fetch movie ids, then fills and prints the DOM tree
		to stdout (in tellico format) so that tellico can use it.
		"""
		self.__conn = sqlite3.connect(self.__dbPath)
		self.__loadDatabase()
		# Print results to stdout
		self.__domTree.printXML()

	def __addMediaValues(self):
		c = self.__conn.cursor()
		c.execute("SELECT name FROM media")

		media = list([row[0].encode('utf-8') for row in c.fetchall()])
		self.__domTree.addMedia(media)


	def __fetchMovieIds(self):
		"""
		Retrieve all movie ids
		"""
		c = self.__conn.cursor()
		c.execute("SELECT movie_id FROM movies")
		data = c.fetchall()
		dataList = [row[0] for row in data]
		return dataList

	def __fetchMovieInfo(self, id):
		"""
		Fetches movie information
		"""
		#cast is a reserved word
		columns = ('title','director','rating','year','region',
					'country','genre','classification','plot',
					'runtime','o_title','studio','notes','image',
					'[cast]','loaned','color','site')

		c = self.__conn.cursor()
		c.execute("SELECT %s FROM movies WHERE movie_id=%s" % (','.join(columns),id))
		row = c.fetchone()

		data = {}
		data['id'] = str(id)

		for i in range(len(columns)):
			if row[i] == None : continue

			try:
				value = row[i].encode('utf-8')
			except:
				value = str(row[i])

			col = columns[i].replace('[','').replace(']','')

			if col == 'genre' or col == 'studio':
				values = value.split('/')
			elif col == 'plot' or col == 'notes':
				value = value.replace('\n', '\n<br/>')
				values = (value,)
			elif col == 'cast':
				values = []
				lines = value.split('\n')
				for line in lines:
					cast = line.split('as')
					values.append(cast)
			elif col == 'image':
				imgfile = POSTERS_PATH + value + '.jpg'
				img = file(imgfile,'rb').read()
				values = ((value + '.jpg', base64.encodestring(img)),)
			elif col == 'loaned':
				if value == '0': value = ''
				values = (value,)
			elif col == 'color':
				if value == '1': value = 'Color'
				elif value == '2': value = 'Black & White'
				values = (value,)
			else:
				values = (value,)
			col = col.replace('"','')
			data[col] = values

		# get medium
		c.execute("SELECT name FROM media WHERE medium_id IN (SELECT medium_id FROM movies WHERE movie_id=%s)" % id)

		media = list([row[0].encode('utf-8') for row in c.fetchall()])
		if len(media) > 0: data['medium'] = media

		# get all tags
		c.execute("SELECT name FROM tags WHERE tag_id IN (SELECT tag_id FROM movie_tag WHERE movie_id=%s)" % id)

		tags = list([row[0].encode('utf-8') for row in c.fetchall()])
		if len(tags) > 0: data['tag'] = tags

		# get all languages
		c.execute("SELECT name FROM languages WHERE lang_id IN (SELECT lang_id FROM movie_lang WHERE movie_id=%s)" % id)

		langs = list([row[0].encode('utf-8') for row in c.fetchall()])
		if len(langs) > 0: data['language'] = langs

		return data


	def __loadDatabase(self):
		# Get all ids
		self.__addMediaValues();
		ids = self.__fetchMovieIds()

		# Now retrieve data
		if ids:
			for entry in ids:
				data = self.__fetchMovieInfo(entry)
				self.__domTree.addEntry(data)
		else:
			return None



def main():
	parser = GriffithParser()
	parser.run()

if __name__ == '__main__':
	main()
